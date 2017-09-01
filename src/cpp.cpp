#include "structs.h"
#include "cpp.h"
#include "helper.h"
#include <QMetaEnum>

template <typename T>
QString cppSetType(const T& p)
{
    if (p.optional) {
        return "option<" + p.type.cppSetType + ">";
    }
    return p.type.cppSetType;
}

QString upperInitial(const QString& name) {
    return name.left(1).toUpper() + name.mid(1);
}

QString lowerInitial(const QString& name) {
    return name.left(1).toLower() + name.mid(1);
}

QString writeProperty(const QString& name) {
    return "WRITE set" + upperInitial(name) + " ";
}

QString baseType(const Object& o) {
    if (o.type != ObjectType::Object) {
        return "QAbstractItemModel";
    }
    return "QObject";
}

QString cGetType(const BindingTypeProperties& type) {
    return type.name + "*, " + type.name.toLower() + "_set";
}

void writeHeaderItemModel(QTextStream& h, const Object& o) {
    h << QString(R"(
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole);
signals:
    // new data is ready to be made available to the model with fetchMore()
    void newDataReady(const QModelIndex &parent) const;
private:
    QHash<QPair<int,Qt::ItemDataRole>, QVariant> m_headerData;
    void initHeaderData();
)");
    for (auto ip: o.itemProperties) {
        if (o.type == ObjectType::List) {
            h << QString("    QVariant %1(int row) const;\n").arg(ip.name);
        } else {
            h << QString("    QVariant %1(const QModelIndex& index) const;\n").arg(ip.name);
        }
/*
        if (ip.write) {
            r << QString("    fn set_%1(&mut self, item: usize, %2) -> bool;\n")
                .arg(snakeCase(ip.name), rustType(ip));
        }
*/
    }
}

bool isColumnWrite(const Object& o, int col) {
    for (auto ip: o.itemProperties) {
        if (ip.write && ip.roles.size() > col && ip.roles[col].size() > 0) {
            return true;
        }
    }
    return false;
}

void writeCppModel(QTextStream& cpp, const Object& o) {
    const QString lcname(snakeCase(o.name));
    QString indexDecl = ", int";
    QString index = ", index.row()";
    if (o.type == ObjectType::UniformTree) {
        indexDecl = ", quintptr";
        index = ", index.internalId()";
    }

    cpp << "extern \"C\" {\n";
    for (auto ip: o.itemProperties) {
        if (ip.type.isComplex()) {
            cpp << QString("    void %2_data_%3(const %1::Private*%5, %4);\n")
                .arg(o.name, lcname, snakeCase(ip.name), cGetType(ip.type), indexDecl);
        } else {
            cpp << QString("    %4 %2_data_%3(const %1::Private*%5);\n")
                .arg(o.name, lcname, snakeCase(ip.name), cppSetType(ip), indexDecl);
        }
        if (ip.write) {
            cpp << QString("    bool %2_set_data_%3(%1::Private*%5, %4);")
                .arg(o.name, lcname, snakeCase(ip.name), ip.type.cSetType, indexDecl) << endl;
            if (ip.optional) {
                cpp << QString("    bool %2_set_data_%3_none(%1::Private*%4);")
                    .arg(o.name, lcname, snakeCase(ip.name), indexDecl) << endl;
            }
        }
    }
    cpp << QString("    void %2_sort(%1::Private*, unsigned char column, Qt::SortOrder order = Qt::AscendingOrder);\n").arg(o.name, lcname);
    if (o.type == ObjectType::List) {
        cpp << QString(R"(
    int %2_row_count(const %1::Private*);
    bool %2_can_fetch_more(const %1::Private*);
    void %2_fetch_more(%1::Private*);
}
int %1::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %3;
}

bool %1::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

int %1::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %2_row_count(m_d);
}

QModelIndex %1::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && row >= 0 && row < rowCount(parent) && column >= 0 && column < %3) {
        return createIndex(row, column, (quintptr)row);
    }
    return QModelIndex();
}

QModelIndex %1::parent(const QModelIndex &) const
{
    return QModelIndex();
}

bool %1::canFetchMore(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %2_can_fetch_more(m_d);
}

void %1::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid()) {
        %2_fetch_more(m_d);
    }
}
)").arg(o.name, lcname, QString::number(o.columnCount));
    } else {
        cpp << QString(R"(
    int %2_row_count(const %1::Private*, quintptr, bool);
    bool %2_can_fetch_more(const %1::Private*, quintptr, bool);
    void %2_fetch_more(%1::Private*, quintptr, bool);
    quintptr %2_index(const %1::Private*, quintptr, bool, int);
    qmodelindex_t %2_parent(const %1::Private*, quintptr);
    int %2_row(const %1::Private*, quintptr);
}
int %1::columnCount(const QModelIndex &) const
{
    return %3;
}

bool %1::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

int %1::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return 0;
    }
    return %2_row_count(m_d, parent.internalId(), parent.isValid());
}

QModelIndex %1::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column >= %3) {
        return QModelIndex();
    }
    if (parent.isValid() && parent.column() != 0) {
        return QModelIndex();
    }
    if (row >= rowCount(parent)) {
        return QModelIndex();
    }
    const quintptr id = %2_index(m_d, parent.internalId(), parent.isValid(), row);
    return createIndex(row, column, id);
}

QModelIndex %1::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const qmodelindex_t parent = %2_parent(m_d, index.internalId());
    return parent.row >= 0 ?createIndex(parent.row, 0, parent.id) :QModelIndex();
}

bool %1::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return false;
    }
    return %2_can_fetch_more(m_d, parent.internalId(), parent.isValid());
}

void %1::fetchMore(const QModelIndex &parent)
{
    %2_fetch_more(m_d, parent.internalId(), parent.isValid());
}
)").arg(o.name, lcname, QString::number(o.columnCount));
    }

    cpp << QString(R"(
void %1::sort(int column, Qt::SortOrder order)
{
    %2_sort(m_d, column, order);
}
Qt::ItemFlags %1::flags(const QModelIndex &i) const
{
    auto flags = QAbstractItemModel::flags(i);
)").arg(o.name, lcname);
    for (int col = 0; col < o.columnCount; ++col) {
        if (isColumnWrite(o, col)) {
            cpp << "    if (i.column() == " << col << ") {\n";
            cpp << "        flags |= Qt::ItemIsEditable;\n    }\n";
        }
    }
    cpp << "    return flags;\n}\n";
    for (auto ip: o.itemProperties) {
        QString idx = index;
        if (o.type == ObjectType::List) {
            idx = ", row";
            cpp << QString("QVariant %1::%2(int row) const\n{\n")
                    .arg(o.name, ip.name);
        } else {
            cpp << QString("QVariant %1::%2(const QModelIndex& index) const\n{\n")
                    .arg(o.name, ip.name);
        }
        cpp << "    QVariant v;\n";
        if (ip.type.name == "QString") {
            cpp << "    QString s;\n";
            cpp << QString("    %1_data_%2(m_d%4, &s, set_%3);\n")
                   .arg(lcname, snakeCase(ip.name), ip.type.name.toLower(), idx);
            cpp << "    if (!s.isNull()) v.setValue<QString>(s);\n";
        } else if (ip.type.name == "QByteArray") {
            cpp << "    QByteArray b;\n";
            cpp << QString("    %1_data_%2(m_d%4, &b, set_%3);\n")
                   .arg(lcname, snakeCase(ip.name), ip.type.name.toLower(), idx);
            cpp << "    if (!b.isNull()) v.setValue<QByteArray>(b);\n";
        } else {
            cpp << QString("    v = %1_data_%2(m_d%3);\n")
                   .arg(lcname, snakeCase(ip.name), idx);
        }
        cpp << "    return v;\n";
        cpp << "}\n";
    }
    cpp << QString(R"(
QVariant %1::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(rowCount(index.parent()) > index.row());
    switch (index.column()) {
)").arg(o.name);

    auto metaRoles = QMetaEnum::fromType<Qt::ItemDataRole>();
    for (int col = 0; col < o.columnCount; ++col) {
        cpp << QString("    case %1:\n").arg(col);
        cpp << QString("        switch (role) {\n");
        for (int i = 0; i < o.itemProperties.size(); ++i) {
            auto ip = o.itemProperties[i];
            auto roles = ip.roles.value(col);
            if (col > 0 && roles.size() == 0) {
                continue;
            }
            for (auto role: roles) {
                cpp << QString("        case Qt::%1:\n").arg(metaRoles.valueToKey(role));
            }
            cpp << QString("        case Qt::UserRole + %1:\n").arg(i);
            if (o.type == ObjectType::List) {
                cpp << QString("            return %1(index.row());\n").arg(ip.name);
            } else {
                cpp << QString("            return %1(index);\n").arg(ip.name);
            }
        }
        cpp << "        }\n";
    }
    cpp << "    }\n    return QVariant();\n}\n";
    cpp << "QHash<int, QByteArray> " << o.name << "::roleNames() const {\n";
    cpp << "    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();\n";
    for (int i = 0; i < o.itemProperties.size(); ++i) {
        auto ip = o.itemProperties[i];
        cpp << "    names.insert(Qt::UserRole + " << i << ", \"" << ip.name << "\");\n";
    }
    cpp << "    return names;\n";
    cpp << QString(R"(}
QVariant %1::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) {
        return QVariant();
    }
    return m_headerData.value(qMakePair(section, (Qt::ItemDataRole)role), role == Qt::DisplayRole ?QString::number(section + 1) :QVariant());
}

bool %1::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (orientation != Qt::Horizontal) {
        return false;
    }
    m_headerData.insert(qMakePair(section, (Qt::ItemDataRole)role), value);
    return true;
}

bool %1::setData(const QModelIndex &index, const QVariant &value, int role)
{
)").arg(o.name);
    cpp << "    bool set = false;\n";
    for (int col = 0; col < o.columnCount; ++col) {
        if (!isColumnWrite(o, col)) {
            continue;
        }
        cpp << "    if (index.column() == " << col << ") {\n";
        for (int i = 0; i < o.itemProperties.size(); ++i) {
            auto ip = o.itemProperties[i];
            if (!ip.write) {
                continue;
            }
            auto roles = ip.roles.value(col);
            if (col > 0 && roles.size() == 0) {
                continue;
            }
            cpp << "        if (";
            for (auto role: roles) {
                cpp << QString("role == Qt::%1 || ").arg(metaRoles.valueToKey(role));
            }
            cpp << "role == Qt::UserRole + " << i << ") {\n";
            if (ip.optional) {
                QString test = "!value.isValid()";
                if (ip.type.isComplex()) {
                    test += " || value.isNull()";
                }
                cpp << "            if (" << test << ") {\n";
                cpp << QString("                set = %1_set_data_%2_none(m_d%3);")
                        .arg(lcname, snakeCase(ip.name), index) << endl;
                cpp << "            } else\n";
            }
            QString val = QString("value.value<%1>()").arg(ip.type.name);
            cpp << QString("            set = %1_set_data_%2(m_d%3, %4);")
                .arg(lcname, snakeCase(ip.name), index, val) << endl;
            cpp << "        }\n";
        }
        cpp << "    }\n";
    }
    cpp << R"(    if (set) {
        emit dataChanged(index, index, QVector<int>() << role);
    }
    return set;
}
)";
}

void writeHeaderObject(QTextStream& h, const Object& o, const Configuration& conf) {
    h << QString(R"(
class %1 : public %3
{
    Q_OBJEC%2
)").arg(o.name, "T", baseType(o));
    for (auto object: conf.objects) {
        if (object.containsObject() && o.name != object.name) {
            h << "    friend class " << object.name << ";\n";
        }
    }
    h << R"(public:
    class Private;
private:
)";
    for (auto p: o.properties) {
        if (p.type.type == BindingType::Object) {
            h << "    " << p.type.name << "* const m_" << p.name << ";\n";
        }
    }
    h << R"(    Private * m_d;
    bool m_ownsPrivate;
)";
    for (auto p: o.properties) {
        bool obj = p.type.type == BindingType::Object;
        h << QString("    Q_PROPERTY(%1 %2 READ %2 %3NOTIFY %2Changed FINAL)")
                .arg(p.type.name + (obj ?"*" :""),
                     p.name,
                     p.write ? writeProperty(p.name) :"")
             << endl;
    }
    h << QString(R"(    explicit %1(bool owned, QObject *parent);
public:
    explicit %1(QObject *parent = nullptr);
    ~%1();
)").arg(o.name);
    for (auto p: o.properties) {
        if (p.type.type == BindingType::Object) {
            h << "    const " << p.type.name << "* " << p.name << "() const;" << endl;
            h << "    " << p.type.name << "* " << p.name << "();" << endl;
        } else {
            h << "    " << p.type.name << " " << p.name << "() const;" << endl;
            if (p.write) {
                h << "    void set" << upperInitial(p.name) << "(" << p.type.cppSetType << " v);" << endl;
            }
        }
    }
    if (baseType(o) == "QAbstractItemModel") {
        writeHeaderItemModel(h, o);
    }
    h << "signals:" << endl;
    for (auto p: o.properties) {
        h << "    void " << p.name << "Changed();" << endl;
    }
    h << "};" << endl;
}

void constructorArgsDecl(QTextStream& cpp, const Object& o, const Configuration& conf) {
    cpp << o.name << "*";
    for (auto p: o.properties) {
        if (p.type.type == BindingType::Object) {
            cpp << QString(", ");
            constructorArgsDecl(cpp, conf.findObject(p.type.name), conf);
        } else {
            cpp << QString(", void (*)(%1*)").arg(o.name);
        }
    }
    if (o.type == ObjectType::List) {
        cpp << QString(R"(,
        void (*)(const %1*),
        void (*)(%1*, quintptr, quintptr),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*))").arg(o.name);
    }
    if (o.type == ObjectType::UniformTree) {
        cpp << QString(R"(,
        void (*)(const %1*, quintptr, bool),
        void (*)(%1*, quintptr, quintptr),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, option<quintptr>, int, int),
        void (*)(%1*),
        void (*)(%1*, option<quintptr>, int, int),
        void (*)(%1*))").arg(o.name);
    }
}

QString changedF(const Object& o, const Property& p) {
    return lowerInitial(o.name) + upperInitial(p.name) + "Changed";
}

void constructorArgs(QTextStream& cpp, const QString& prefix, const Object& o, const Configuration& conf) {
    const QString lcname(snakeCase(o.name));
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            cpp << ", " << prefix << "m_" << p.name;
            constructorArgs(cpp, "m_" + p.name + "->",
                    conf.findObject(p.type.name), conf);
        } else {
            cpp << ",\n        " << changedF(o, p);
        }
    }
    if (o.type == ObjectType::List) {
        cpp << QString(R"(,
        [](const %1* o) {
            emit o->newDataReady(QModelIndex());
        },
        [](%1* o, quintptr first, quintptr last) {
            o->dataChanged(o->createIndex(first, 0, first),
                       o->createIndex(last, %2, last));
        },
        [](%1* o) {
            o->beginResetModel();
        },
        [](%1* o) {
            o->endResetModel();
        },
        [](%1* o, int first, int last) {
            o->beginInsertRows(QModelIndex(), first, last);
        },
        [](%1* o) {
            o->endInsertRows();
        },
        [](%1* o, int first, int last) {
            o->beginRemoveRows(QModelIndex(), first, last);
        },
        [](%1* o) {
            o->endRemoveRows();
        }
)").arg(o.name,     QString::number(o.columnCount - 1));
    }
    if (o.type == ObjectType::UniformTree) {
        cpp << QString(R"(,
        [](const %1* o, quintptr id, bool valid) {
            if (valid) {
                int row = %2_row(o->m_d, id);
                emit o->newDataReady(o->createIndex(row, 0, id));
            } else {
                emit o->newDataReady(QModelIndex());
            }
        },
        [](%1* o, quintptr first, quintptr last) {
            quintptr frow = %2_row(o->m_d, first);
            quintptr lrow = %2_row(o->m_d, first);
            o->dataChanged(o->createIndex(frow, 0, first),
                       o->createIndex(lrow, %3, last));
        },
        [](%1* o) {
            o->beginResetModel();
        },
        [](%1* o) {
            o->endResetModel();
        },
        [](%1* o, option<quintptr> id, int first, int last) {
            if (id.some) {
                int row = %2_row(o->m_d, id.value);
                o->beginInsertRows(o->createIndex(row, 0, id.value), first, last);
            } else {
                o->beginInsertRows(QModelIndex(), first, last);
            }
        },
        [](%1* o) {
            o->endInsertRows();
        },
        [](%1* o, option<quintptr> id, int first, int last) {
            if (id.some) {
                int row = %2_row(o->m_d, id.value);
                o->beginRemoveRows(o->createIndex(row, 0, id.value), first, last);
            } else {
                o->beginRemoveRows(QModelIndex(), first, last);
            }
        },
        [](%1* o) {
            o->endRemoveRows();
        }
)").arg(o.name, lcname, QString::number(o.columnCount - 1));
    }
}

void writeObjectCDecl(QTextStream& cpp, const Object& o, const Configuration& conf) {
    const QString lcname(snakeCase(o.name));
    cpp << QString("    %1::Private* %2_new(").arg(o.name, lcname);
    constructorArgsDecl(cpp, o, conf);
    cpp << ");" << endl;
    cpp << QString("    void %2_free(%1::Private*);").arg(o.name, lcname)
        << endl;
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        if (p.type.type == BindingType::Object) {
            cpp << QString("    %3::Private* %2_get(const %1::Private*);")
                .arg(o.name, base, p.type.name) << endl;
        } else if (p.type.isComplex()) {
            cpp << QString("    void %2_get(const %1::Private*, %3);")
                .arg(o.name, base, cGetType(p.type)) << endl;
        } else {
            cpp << QString("    %3 %2_get(const %1::Private*);")
                .arg(o.name, base, p.type.name) << endl;
        }
        if (p.write) {
            cpp << QString("    void %2_set(%1::Private*, %3);")
                .arg(o.name, base, p.type.cSetType) << endl;
            if (p.optional) {
                cpp << QString("    void %2_set_none(%1::Private*);")
                    .arg(o.name, base) << endl;
            }
        }
    }
}

void initializeMembersEmpty(QTextStream& cpp, const Object& o, const Configuration& conf)
{
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            initializeMembersEmpty(cpp, conf.findObject(p.type.name), conf);
            cpp << QString("    %1m_%2(new %3(false, this)),\n")
                   .arg(p.name, p.type.name);
        }
    }
}

void initializeMembersZero(QTextStream& cpp, const Object& o)
{
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            cpp << QString("    m_%1(new %2(false, this)),\n")
                   .arg(p.name, p.type.name);
        }
    }
}

void initializeMembers(QTextStream& cpp, const QString& prefix, const Object& o, const Configuration& conf)
{
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            cpp << QString("    %1m_%2->m_d = %3_%4_get(%1m_d);\n")
                   .arg(prefix, p.name, snakeCase(o.name), snakeCase(p.name));
            initializeMembers(cpp, "m_" + p.name + "->",
                    conf.findObject(p.type.name), conf);
        }
    }
}

void connect(QTextStream& cpp, const QString& d, const Object& o, const Configuration& conf) {
    for (auto p: o.properties) {
        if (p.type.type == BindingType::Object) {
            connect(cpp, d + "->m_" + p.name, conf.findObject(p.type.name), conf);
        }
    }
    if (o.type != ObjectType::Object) {
        cpp << QString(R"(    connect(%2, &%1::newDataReady, %2, [this](const QModelIndex& i) {
        %2->fetchMore(i);
    }, Qt::QueuedConnection);
)").arg(o.name, d);
    }
}

void writeCppObject(QTextStream& cpp, const Object& o, const Configuration& conf) {
    const QString lcname(snakeCase(o.name));
    cpp << QString("%1::%1(bool /*owned*/, QObject *parent):\n    %2(parent),")
            .arg(o.name, baseType(o)) << endl;
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            cpp << QString("    m_%1(new %2(false, this)),\n")
                   .arg(p.name, p.type.name);
        }
    }
    cpp << "    m_d(0),\n    m_ownsPrivate(false)\n{\n";
    if (o.type != ObjectType::Object) {
        cpp << "    initHeaderData();\n";
    }
    cpp << QString("}\n\n%1::%1(QObject *parent):\n    %2(parent),")
            .arg(o.name, baseType(o)) << endl;
    initializeMembersZero(cpp, o);
    cpp << QString("    m_d(%1_new(this").arg(lcname);
    constructorArgs(cpp, "", o, conf);
    cpp << ")),\n    m_ownsPrivate(true)\n{\n";
    initializeMembers(cpp, "", o, conf);
    connect(cpp, "this", o, conf);
    if (o.type != ObjectType::Object) {
        cpp << "    initHeaderData();\n";
    }
    cpp << QString(R"(}

%1::~%1() {
    if (m_ownsPrivate) {
        %2_free(m_d);
    }
}
)").arg(o.name, lcname);

    if (o.type != ObjectType::Object) {
        cpp << QString("void %1::initHeaderData() {\n").arg(o.name);

        for (int col = 0; col < o.columnCount; ++col) {
            for (auto ip: o.itemProperties) {
                auto roles = ip.roles.value(col);
                if (roles.contains(Qt::DisplayRole)) {
                    cpp << QString("    m_headerData.insert(qMakePair(%1, Qt::DisplayRole), QVariant(\"%2\"));\n").arg(QString::number(col), ip.name);
                }
            }
        }
        cpp << "    }\n";
    }

    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        if (p.type.type == BindingType::Object) {
            cpp << QString(R"(const %3* %1::%2() const
{
    return m_%2;
}
%3* %1::%2()
{
    return m_%2;
}
)").arg(o.name, p.name, p.type.name);
        } else if (p.type.isComplex()) {
            cpp << QString("%3 %1::%2() const\n{\n").arg(o.name, p.name, p.type.name);
            cpp << "    " << p.type.name << " v;\n";
            cpp << "    " << base << "_get(m_d, &v, set_" << p.type.name.toLower()
                << ");\n";
            cpp << "    return v;\n}\n";
        } else {
            cpp << QString("%3 %1::%2() const\n{\n").arg(o.name, p.name, p.type.name);
            cpp << QString("    return %1_get(m_d);\n}\n").arg(base);
        }
        if (p.write) {
            cpp << "void " << o.name << "::set" << upperInitial(p.name) << "(" << p.type.cppSetType << " v) {" << endl;
            if (p.optional) {
                cpp << QString("    if (v.isNull()) {") << endl;
                cpp << QString("        %1_set_none(m_d);").arg(base) << endl;
                cpp << QString("    } else {") << endl;
                cpp << QString("        %1_set(m_d, v);").arg(base) << endl;
                cpp << QString("    }") << endl;
            } else {
                cpp << QString("    %1_set(m_d, v);").arg(base) << endl;
            }
            cpp << "}" << endl;
        }
    }
}

void writeHeader(const Configuration& conf) {
    DifferentFileWriter w(conf.hFile.absoluteFilePath());
    QTextStream h(&w.buffer);
    const QString guard(conf.hFile.fileName().replace('.', '_').toUpper());
    h << QString(R"(/* generated by rust_qt_binding_generator */
#ifndef %1
#define %1

#include <QObject>
#include <QAbstractItemModel>

)").arg(guard);

    for (auto object: conf.objects) {
        h << "class " << object.name << ";\n";
    }
    for (auto object: conf.objects) {
        writeHeaderObject(h, object, conf);
    }

    h << QString("#endif // %1\n").arg(guard);
}

void writeCpp(const Configuration& conf) {
    DifferentFileWriter w(conf.cppFile.absoluteFilePath());
    QTextStream cpp(&w.buffer);
    cpp << QString(R"(/* generated by rust_qt_binding_generator */
#include "%1"

namespace {
    template <typename T>
    struct option {
    public:
        T value;
        bool some;
        operator QVariant() const {
            if (some) {
                return QVariant(value);
            }
            return QVariant();
        }
    };
    struct qbytearray_t {
    private:
        const char* data;
        int len;
    public:
        qbytearray_t(const QByteArray& v):
            data(v.data()),
            len(v.size()) {
        }
        operator QByteArray() const {
            return QByteArray(data, len);
        }
    };
    struct qstring_t {
    private:
        const void* data;
        int len;
    public:
        qstring_t(const QString& v):
            data(static_cast<const void*>(v.utf16())),
            len(v.size()) {
        }
        operator QString() const {
            return QString::fromUtf8(static_cast<const char*>(data), len);
        }
    };
    struct qmodelindex_t {
        int row;
        quintptr id;
    };
)").arg(conf.hFile.fileName());

    for (auto o: conf.objects) {
        for (auto p: o.properties) {
            if (p.type.type == BindingType::Object) {
                continue;
            }
            cpp << "    inline void " << changedF(o, p) << "(" << o.name << "* o)\n";
            cpp << "    {\n        emit o->" << p.name << "Changed();\n    }\n";
        }
    }

    cpp << R"(
}
typedef void (*qstring_set)(QString*, qstring_t*);
void set_qstring(QString* v, qstring_t* val) {
    *v = *val;
}
typedef void (*qbytearray_set)(QByteArray*, qbytearray_t*);
void set_qbytearray(QByteArray* v, qbytearray_t* val) {
    *v = *val;
}
)";

    for (auto object: conf.objects) {
        if (object.type != ObjectType::Object) {
            writeCppModel(cpp, object);
        }

        cpp << "extern \"C\" {\n";
        writeObjectCDecl(cpp, object, conf);
        cpp << "};\n" << endl;
    }

    for (auto object: conf.objects) {
        writeCppObject(cpp, object, conf);
    }
}
