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

void writeHeaderItemModel(QTextStream& h) {
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
signals:
    // new data is ready to be made available to the model with fetchMore()
    void newDataReady(const QModelIndex &parent) const;
)");
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
        indexDecl = ", int, quintptr";
        index = ", index.row(), index.internalId()";
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
    return (parent.isValid()) ? 0 : %2_row_count(d);
}

QModelIndex %1::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && row >= 0 && row < rowCount(parent) && column >= 0 && column < %3) {
        return createIndex(row, column, (quintptr)0);
    }
    return QModelIndex();
}

QModelIndex %1::parent(const QModelIndex &) const
{
    return QModelIndex();
}

bool %1::canFetchMore(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %2_can_fetch_more(d);
}

void %1::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid()) {
        %2_fetch_more(d);
    }
}
)").arg(o.name, lcname, QString::number(o.columnCount));
    } else {
        cpp << QString(R"(
    int %2_row_count(const %1::Private*, int, quintptr);
    bool %2_can_fetch_more(const %1::Private*, int, quintptr);
    void %2_fetch_more(%1::Private*, int, quintptr);
    quintptr %2_index(const %1::Private*, int, quintptr);
    qmodelindex_t %2_parent(const %1::Private*, quintptr);
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
    return %2_row_count(d, parent.row(), parent.internalId());
}

QModelIndex %1::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column >= %3) {
        return QModelIndex();
    }
    if (parent.isValid() && parent.column() != 0) {
        return QModelIndex();
    }
    const quintptr id = %2_index(d, parent.row(), parent.internalId());
    return createIndex(row, column, id);
}

QModelIndex %1::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const qmodelindex_t parent = %2_parent(d, index.internalId());
    return parent.row >= 0 ?createIndex(parent.row, 0, parent.id) :QModelIndex();
}

bool %1::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return false;
    }
    return %2_can_fetch_more(d, parent.row(), parent.internalId());
}

void %1::fetchMore(const QModelIndex &parent)
{
    %2_fetch_more(d, parent.row(), parent.internalId());
}
)").arg(o.name, lcname, QString::number(o.columnCount));
    }

    cpp << QString(R"(
void %1::sort(int column, Qt::SortOrder order)
{
    %2_sort(d, column, order);
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
    cpp << QString(R"(    return flags;
}
QVariant %1::data(const QModelIndex &index, int role) const
{
    QVariant v;
    QString s;
    QByteArray b;
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
            if (ip.type.name == "QString") {
                cpp << QString("            %1_data_%2(d%4, &s, set_%3);\n")
                       .arg(lcname, snakeCase(ip.name), ip.type.name.toLower(), index);
                cpp << "            if (!s.isNull()) v.setValue<QString>(s);\n";
            } else if (ip.type.name == "QByteArray") {
                cpp << QString("            %1_data_%2(d%4, &b, set_%3);\n")
                       .arg(lcname, snakeCase(ip.name), ip.type.name.toLower(), index);
                cpp << "            if (!b.isNull()) v.setValue<QByteArray>(b);\n";
            } else {
                cpp << QString("            v = %1_data_%2(d%3);\n")
                       .arg(lcname, snakeCase(ip.name), index);
            }
            cpp << "            break;\n";
        }
        cpp << "        }\n";
        cpp << "        break;\n";
    }
    cpp << "    }\n    return v;\n}\n";
    cpp << "QHash<int, QByteArray> " << o.name << "::roleNames() const {\n";
    cpp << "    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();\n";
    for (int i = 0; i < o.itemProperties.size(); ++i) {
        auto ip = o.itemProperties[i];
        cpp << "    names.insert(Qt::UserRole + " << i << ", \"" << ip.name << "\");\n";
    }
    cpp << "    return names;\n";
    cpp << QString(R"(}
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
                cpp << QString("                set = %1_set_data_%2_none(d%3);")
                        .arg(lcname, snakeCase(ip.name), index) << endl;
                cpp << "            } else\n";
            }
            QString val = QString("value.value<%1>()").arg(ip.type.name);
            cpp << QString("            set = %1_set_data_%2(d%3, %4);")
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

void writeHeaderObject(QTextStream& h, const Object& o) {
    h << QString(R"(
class %1 : public %3
{
    Q_OBJEC%2
public:
    class Private;
private:
    Private * const d;
)").arg(o.name, "T", baseType(o));
    for (auto p: o.properties) {
        h << QString("    Q_PROPERTY(%1 %2 READ %2 %3NOTIFY %2Changed FINAL)")
                .arg(p.type.name, p.name,
                     p.write ? writeProperty(p.name) :"") << endl;
    }
    h << QString(R"(public:
    explicit %1(QObject *parent = nullptr);
    ~%1();
)").arg(o.name);
    for (auto p: o.properties) {
        h << "    " << p.type.name << " " << p.name << "() const;" << endl;
        if (p.write) {
            h << "    void set" << upperInitial(p.name) << "(" << p.type.cppSetType << " v);" << endl;
        }
    }
    if (baseType(o) == "QAbstractItemModel") {
        writeHeaderItemModel(h);
    }
    h << "signals:" << endl;
    for (auto p: o.properties) {
        h << "    void " << p.name << "Changed();" << endl;
    }
    h << "private:" << endl;
    for (auto p: o.properties) {
        h << "    " << p.type.name << " m_" << p.name << ";" << endl;
    }
    h << "};" << endl;
}

void writeObjectCDecl(QTextStream& cpp, const Object& o) {
    const QString lcname(snakeCase(o.name));
    cpp << QString("    %1::Private* %2_new(%1*").arg(o.name, lcname);
    for (int i = 0; i < o.properties.size(); ++i) {
        cpp << QString(", void (*)(%1*)").arg(o.name);
    }
    if (o.type == ObjectType::List) {
        cpp << QString(R"(,
        void (*)(const %1*),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*))").arg(o.name);
    }
    if (o.type == ObjectType::UniformTree) {
        cpp << QString(R"(,
        void (*)(const %1*, quintptr),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, quintptr, int, int),
        void (*)(%1*),
        void (*)(%1*, quintptr, int, int),
        void (*)(%1*))").arg(o.name);
    }
    cpp << ");" << endl;
    cpp << QString("    void %2_free(%1::Private*);").arg(o.name, lcname)
        << endl;
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        if (p.type.isComplex()) {
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

void writeCppObject(QTextStream& cpp, const Object& o) {
    const QString lcname(snakeCase(o.name));
    cpp << QString("%1::%1(QObject *parent):\n    %2(parent),")
            .arg(o.name, baseType(o)) << endl;
    cpp << QString("    d(%1_new(this").arg(lcname);
    for (const Property& p: o.properties) {
        cpp << QString(",\n        [](%1* o) { emit o->%2Changed(); }")
            .arg(o.name, p.name);
    }
    if (o.type == ObjectType::List) {
        cpp << QString(R"(,
        [](const %1* o) {
            emit o->newDataReady(QModelIndex());
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
    )) {
    connect(this, &%1::newDataReady, this, [this](const QModelIndex& i) {
        fetchMore(i);
    }, Qt::QueuedConnection);
}
)").arg(o.name);
    }
    if (o.type == ObjectType::UniformTree) {
        cpp << QString(R"(,
        [](const %1* o, quintptr id) {
            auto i = %2_parent(o->d, id);
            emit o->newDataReady(o->createIndex(i.row, 0, i.id));
        },
        [](%1* o) {
            o->beginResetModel();
        },
        [](%1* o) {
            o->endResetModel();
        },
        [](%1* o, quintptr id, int first, int last) {
                       auto i = %2_parent(o->d, id);
            o->beginInsertRows(o->createIndex(i.row, 0, i.id), first, last);
        },
        [](%1* o) {
            o->endInsertRows();
        },
        [](%1* o, quintptr id, int first, int last) {
                       auto i = %2_parent(o->d, id);
            o->beginRemoveRows(o->createIndex(i.row, 0, i.id), first, last);
        },
        [](%1* o) {
            o->endRemoveRows();
        }
    )) {
    connect(this, &%1::newDataReady, this, [this](const QModelIndex& i) {
        fetchMore(i);
    }, Qt::QueuedConnection);
}
)").arg(o.name, lcname);
    }
    if (o.type == ObjectType::Object) {
        cpp << QString(")) {}");
    }
    cpp << QString(R"(

%1::~%1() {
    %2_free(d);
}
)").arg(o.name, lcname);

    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        cpp << QString("%3 %1::%2() const\n{\n").arg(o.name, p.name, p.type.name);
        if (p.type.isComplex()) {
            cpp << "    " << p.type.name << " v;\n";
            cpp << "    " << base << "_get(d, &v, set_" << p.type.name.toLower()
                << ");\n";
            cpp << "    return v;\n}\n";
        } else {
            cpp << QString("    return %1_get(d);\n}\n").arg(base);
        }
        if (p.write) {
            cpp << "void " << o.name << "::set" << upperInitial(p.name) << "(" << p.type.cppSetType << " v) {" << endl;
            if (p.optional) {
                cpp << QString("    if (v.isNull()) {") << endl;
                cpp << QString("        %1_set_none(d);").arg(base) << endl;
                cpp << QString("    } else {") << endl;
                cpp << QString("        %1_set(d, v);").arg(base) << endl;
                cpp << QString("    }") << endl;
            } else {
                cpp << QString("    %1_set(d, v);").arg(base) << endl;
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
        writeHeaderObject(h, object);
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
    private:
        T value;
        bool some;
    public:
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
}
typedef void (*qstring_set)(QString*, qstring_t*);
void set_qstring(QString* v, qstring_t* val) {
    *v = *val;
}
typedef void (*qbytearray_set)(QByteArray*, qbytearray_t*);
void set_qbytearray(QByteArray* v, qbytearray_t* val) {
    *v = *val;
}

)").arg(conf.hFile.fileName());

    for (auto object: conf.objects) {
        if (object.type != ObjectType::Object) {
            writeCppModel(cpp, object);
        }

        cpp << "extern \"C\" {\n";
        writeObjectCDecl(cpp, object);
        cpp << "};" << endl;
    }

    for (auto object: conf.objects) {
        writeCppObject(cpp, object);
    }
}
