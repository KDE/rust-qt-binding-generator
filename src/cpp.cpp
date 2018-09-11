/*
 *   Copyright 2017  Jos van den Oever <jos@vandenoever.info>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "structs.h"
#include "cpp.h"
#include "helper.h"
#include <QMetaEnum>
#include <QDebug>

template <typename T>
QString cppSetType(const T& p)
{
    if (p.optional) {
        return "option_" + p.type.cppSetType;
    }
    return p.type.cppSetType;
}

template <typename T>
QString propertyType(const T& p)
{
    return (p.optional && !p.type.isComplex()) ?"QVariant" :p.type.name;
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

bool modelIsWritable(const Object& o) {
    bool write = false;
    for (auto ip: o.itemProperties) {
        write |= ip.write;
    }
    return write;
}

void writeHeaderItemModel(QTextStream& h, const Object& o) {
    h << QString(R"(
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    int role(const char* name) const;
    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
)");
    if (modelIsWritable(o)) {
        h << "    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;\n";
    }
    for (auto ip: o.itemProperties) {
        auto r = propertyType(ip);
        auto rw = r;
        if (r == "QVariant" || ip.type.isComplex()) {
            rw = "const " + r + "&";
        }
        if (o.type == ObjectType::List) {
            h << QString("    Q_INVOKABLE %2 %1(int row) const;\n").arg(ip.name, r);
            if (ip.write) {
                h << QString("    Q_INVOKABLE bool set%1(int row, %2 value);\n").arg(upperInitial(ip.name), rw);
            }
        } else {
            h << QString("    Q_INVOKABLE %2 %1(const QModelIndex& index) const;\n").arg(ip.name, r);
            if (ip.write) {
                h << QString("    Q_INVOKABLE bool set%1(const QModelIndex& index, %2 value);\n").arg(upperInitial(ip.name), rw);
            }
        }
    }
    h << R"(
signals:
    // new data is ready to be made available to the model with fetchMore()
    void newDataReady(const QModelIndex &parent) const;
private:
    QHash<QPair<int,Qt::ItemDataRole>, QVariant> m_headerData;
    void initHeaderData();
    void updatePersistentIndexes();
)";
}

bool isColumnWrite(const Object& o, int col) {
    for (auto ip: o.itemProperties) {
        if (ip.write && (col == 0 || (ip.roles.size() > col && ip.roles[col].size() > 0))) {
            return true;
        }
    }
    return false;
}

void writeModelGetterSetter(QTextStream& cpp, const QString& index,
        const ItemProperty& ip, const Object& o) {
    const QString lcname(snakeCase(o.name));
    QString idx = index;

    // getter
    auto r = propertyType(ip);
    if (o.type == ObjectType::List) {
        idx = ", row";
        cpp << QString("%3 %1::%2(int row) const\n{\n")
                .arg(o.name, ip.name, r);
    } else {
        cpp << QString("%3 %1::%2(const QModelIndex& index) const\n{\n")
                .arg(o.name, ip.name, r);
    }
    if (ip.type.name == "QString") {
        cpp << "    QString s;\n";
        cpp << QString("    %1_data_%2(m_d%4, &s, set_%3);\n")
               .arg(lcname, snakeCase(ip.name), ip.type.name.toLower(), idx);
        cpp << "    return s;\n";
    } else if (ip.type.name == "QByteArray") {
        cpp << "    QByteArray b;\n";
        cpp << QString("    %1_data_%2(m_d%4, &b, set_%3);\n")
               .arg(lcname, snakeCase(ip.name), ip.type.name.toLower(), idx);
        cpp << "    return b;\n";
    } else if (ip.optional) {
        cpp << "    QVariant v;\n";
        cpp << QString("    v = %1_data_%2(m_d%3);\n")
               .arg(lcname, snakeCase(ip.name), idx);
        cpp << "    return v;\n";
    } else {
        cpp << QString("    return %1_data_%2(m_d%3);\n")
               .arg(lcname, snakeCase(ip.name), idx);
    }
    cpp << "}\n\n";

    if (!ip.write) {
        return;
    }

    // setter
    if (r == "QVariant" || ip.type.isComplex()) {
        r = "const " + r + "&";
    }
    if (o.type == ObjectType::List) {
        idx = ", row";
        cpp << QString("bool %1::set%2(int row, %3 value)\n{\n")
                .arg(o.name, upperInitial(ip.name), r);
    } else {
        cpp << QString("bool %1::set%2(const QModelIndex& index, %3 value)\n{\n")
                .arg(o.name, upperInitial(ip.name), r);
    }
    cpp << "    bool set = false;\n";
    if (ip.optional) {
        QString test = "value.isNull()";
        if (!ip.type.isComplex()) {
            test += " || !value.isValid()";
        }
        cpp << "    if (" << test << ") {\n";
        cpp << QString("        set = %1_set_data_%2_none(m_d%3);")
                .arg(lcname, snakeCase(ip.name), idx) << endl;
        cpp << "    } else {\n";
    }
    if (ip.optional && !ip.type.isComplex()) {
        cpp << QString("    if (!value.canConvert(qMetaTypeId<%1>())) {\n        return false;\n    }\n").arg(ip.type.name);
        cpp << QString("    set = %1_set_data_%2(m_d%3, value.value<%4>());")
            .arg(lcname, snakeCase(ip.name), idx, ip.type.name) << endl;
    } else {
        QString val = "value";
        if (ip.type.isComplex()) {
            if (ip.type.name == "QString") {
                val = "value.utf16(), value.length()";
            } else {
                val = "value.data(), value.length()";
            }
        }
        cpp << QString("    set = %1_set_data_%2(m_d%3, %4);")
            .arg(lcname, snakeCase(ip.name), idx, val) << endl;
    }
    if (ip.optional) {
        cpp << "    }\n";
    }
    if (o.type == ObjectType::List) {
        cpp << R"(    if (set) {
        QModelIndex index = createIndex(row, 0, row);
        emit dataChanged(index, index);
    }
    return set;
}

)";
    } else {
        cpp << R"(    if (set) {
        emit dataChanged(index, index);
    }
    return set;
}

)";
    }
}

void writeCppModel(QTextStream& cpp, const Object& o) {
    const QString lcname(snakeCase(o.name));
    QString indexDecl = ", int";
    QString index = ", index.row()";
    if (o.type == ObjectType::Tree) {
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
            if (ip.type.name == "QString") {
                cpp << QString("    bool %2_set_data_%3(%1::Private*%4, const ushort* s, int len);")
                    .arg(o.name, lcname, snakeCase(ip.name), indexDecl) << endl;
            } else if (ip.type.name == "QByteArray") {
                cpp << QString("    bool %2_set_data_%3(%1::Private*%4, const char* s, int len);")
                    .arg(o.name, lcname, snakeCase(ip.name), indexDecl) << endl;
            } else {
                cpp << QString("    bool %2_set_data_%3(%1::Private*%5, %4);")
                    .arg(o.name, lcname, snakeCase(ip.name), ip.type.cSetType, indexDecl) << endl;
            }
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
    bool %2_insert_rows(%1::Private*, int, int);
    bool %2_remove_rows(%1::Private*, int, int);
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

bool %1::insertRows(int row, int count, const QModelIndex &)
{
    return %2_insert_rows(m_d, row, count);
}

bool %1::removeRows(int row, int count, const QModelIndex &)
{
    return %2_remove_rows(m_d, row, count);
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
void %1::updatePersistentIndexes() {}
)").arg(o.name, lcname, QString::number(o.columnCount));
    } else {
        cpp << QString(R"(
    int %2_row_count(const %1::Private*, option_quintptr);
    bool %2_can_fetch_more(const %1::Private*, option_quintptr);
    void %2_fetch_more(%1::Private*, option_quintptr);
    quintptr %2_index(const %1::Private*, option_quintptr, int);
    qmodelindex_t %2_parent(const %1::Private*, quintptr);
    int %2_row(const %1::Private*, quintptr);
    option_quintptr %2_check_row(const %1::Private*, quintptr, int);
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
    const option_quintptr rust_parent = {
        parent.internalId(),
        parent.isValid()
    };
    return %2_row_count(m_d, rust_parent);
}

bool %1::insertRows(int, int, const QModelIndex &)
{
    return false; // not supported yet
}

bool %1::removeRows(int, int, const QModelIndex &)
{
    return false; // not supported yet
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
    const option_quintptr rust_parent = {
        parent.internalId(),
        parent.isValid()
    };
    const quintptr id = %2_index(m_d, rust_parent, row);
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
    const option_quintptr rust_parent = {
        parent.internalId(),
        parent.isValid()
    };
    return %2_can_fetch_more(m_d, rust_parent);
}

void %1::fetchMore(const QModelIndex &parent)
{
    const option_quintptr rust_parent = {
        parent.internalId(),
        parent.isValid()
    };
    %2_fetch_more(m_d, rust_parent);
}
void %1::updatePersistentIndexes() {
    const auto from = persistentIndexList();
    auto to = from;
    auto len = to.size();
    for (int i = 0; i < len; ++i) {
        auto index = to.at(i);
        auto row = %2_check_row(m_d, index.internalId(), index.row());
        if (row.some) {
            to[i] = createIndex(row.value, index.column(), index.internalId());
        } else {
            to[i] = QModelIndex();
        }
    }
    changePersistentIndexList(from, to);
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
    cpp << "    return flags;\n}\n\n";
    for (auto ip: o.itemProperties) {
        writeModelGetterSetter(cpp, index, ip, o);
    }
    cpp << QString(R"(QVariant %1::data(const QModelIndex &index, int role) const
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
            auto ii = (o.type == ObjectType::List) ?".row()" :"";
            if (ip.optional && !ip.type.isComplex()) {
                cpp << QString("            return %1(index%2);\n").arg(ip.name, ii);
            } else if (ip.optional) {
                cpp << QString("            return cleanNullQVariant(QVariant::fromValue(%1(index%2)));\n").arg(ip.name, ii);
            } else {
                cpp << QString("            return QVariant::fromValue(%1(index%2));\n").arg(ip.name, ii);
            }
        }
        cpp << "        }\n";
    }
    cpp << "    }\n    return QVariant();\n}\n\n";
    cpp << "int " << o.name << "::role(const char* name) const {\n";
    cpp << "    auto names = roleNames();\n";
    cpp << "    auto i = names.constBegin();\n";
    cpp << "    while (i != names.constEnd()) {\n";
    cpp << "        if (i.value() == name) {\n";
    cpp << "            return i.key();\n";
    cpp << "        }\n";
    cpp << "        ++i;\n";
    cpp << "    }\n";
    cpp << "    return -1;\n";
    cpp << "}\n";
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

)").arg(o.name);
    if (modelIsWritable(o)) {
        cpp << QString("bool %1::setData(const QModelIndex &index, const QVariant &value, int role)\n{\n").arg(o.name);
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
                auto ii = (o.type == ObjectType::List) ?".row()" :"";
                if (ip.optional && !ip.type.isComplex()) {
                    cpp << QString("            return set%1(index%2, value);\n")
                            .arg(upperInitial(ip.name), ii);
                } else {
                    QString pre = "";
                    if (ip.optional) {
                        pre = "!value.isValid() || value.isNull() ||";
                    }
                    cpp << QString("            if (%2value.canConvert(qMetaTypeId<%1>())) {\n").arg(ip.type.name, pre);
                    cpp << QString("                return set%1(index%2, value.value<%3>());\n").arg(upperInitial(ip.name), ii, ip.type.name);
                    cpp << QString("            }\n");
                }
                cpp << "        }\n";
            }
            cpp << "    }\n";
        }
        cpp << "    return false;\n}\n\n";
    }
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
        auto t = p.type.name;
        if (p.optional && !p.type.isComplex()) {
            t = "QVariant";
        }
        h << QString("    Q_PROPERTY(%1 %2 READ %2 %3NOTIFY %2Changed FINAL)")
                .arg(t + (obj ?"*" :""),
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
            auto t = p.type.name;
            auto t2 = p.type.cppSetType;
            if (p.optional && !p.type.isComplex()) {
                t = "QVariant";
                t2 = "const QVariant&";
            }
            h << "    " << t << " " << p.name << "() const;" << endl;
            if (p.write) {
                h << "    void set" << upperInitial(p.name) << "(" << t2 << " v);" << endl;
            }
        }
    }
    for (auto f: o.functions) {
        h << "    Q_INVOKABLE " << f.type.name << " " << f.name << "(";
        for (auto a = f.args.begin(); a < f.args.end(); a++) {
            if (a != f.args.begin()) {
                h << ", ";
            }
            h << QString("%1 %2").arg(a->type.cppSetType, a->name);
        }
        h << QString(")%1;").arg(f.mut ? "" : " const");
        h << endl;
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
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, quintptr, quintptr),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*),
        void (*)(%1*, int, int, int),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*))").arg(o.name);
    }
    if (o.type == ObjectType::Tree) {
        cpp << QString(R"(,
        void (*)(const %1*, option_quintptr),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, quintptr, quintptr),
        void (*)(%1*),
        void (*)(%1*),
        void (*)(%1*, option_quintptr, int, int),
        void (*)(%1*),
        void (*)(%1*, option_quintptr, int, int, option_quintptr, int),
        void (*)(%1*),
        void (*)(%1*, option_quintptr, int, int),
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
        [](%1* o) {
            emit o->layoutAboutToBeChanged();
        },
        [](%1* o) {
            o->updatePersistentIndexes();
            emit o->layoutChanged();
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
        [](%1* o, int first, int last, int destination) {
            o->beginMoveRows(QModelIndex(), first, last, QModelIndex(), destination);
        },
        [](%1* o) {
            o->endMoveRows();
        },
        [](%1* o, int first, int last) {
            o->beginRemoveRows(QModelIndex(), first, last);
        },
        [](%1* o) {
            o->endRemoveRows();
        }
)").arg(o.name,     QString::number(o.columnCount - 1));
    }
    if (o.type == ObjectType::Tree) {
        cpp << QString(R"(,
        [](const %1* o, option_quintptr id) {
            if (id.some) {
                int row = %2_row(o->m_d, id.value);
                emit o->newDataReady(o->createIndex(row, 0, id.value));
            } else {
                emit o->newDataReady(QModelIndex());
            }
        },
        [](%1* o) {
            emit o->layoutAboutToBeChanged();
        },
        [](%1* o) {
            o->updatePersistentIndexes();
            emit o->layoutChanged();
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
        [](%1* o, option_quintptr id, int first, int last) {
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
        [](%1* o, option_quintptr sourceParent, int first, int last, option_quintptr destinationParent, int destination) {
            QModelIndex s;
            if (sourceParent.some) {
                int row = %2_row(o->m_d, sourceParent.value);
                s = o->createIndex(row, 0, sourceParent.value);
            }
            QModelIndex d;
            if (destinationParent.some) {
                int row = %2_row(o->m_d, destinationParent.value);
                d = o->createIndex(row, 0, destinationParent.value);
            }
            o->beginMoveRows(s, first, last, d, destination);
        },
        [](%1* o) {
            o->endMoveRows();
        },
        [](%1* o, option_quintptr id, int first, int last) {
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

void writeFunctionCDecl(QTextStream& cpp, const Function& f, const QString& lcname, const Object& o) {
    const QString lc(snakeCase(f.name));
    cpp << "    ";
    if (f.type.isComplex()) {
        cpp << "void";
    } else {
        cpp << f.type.name;
    }
    const QString name = QString("%1_%2").arg(lcname, lc);
    cpp << QString(" %1(%3%2::Private*").arg(name, o.name, f.mut ? "" : "const ");
    // write all the input arguments, for QString and QByteArray, write
    // pointers to their content and the length
    for (auto a = f.args.begin(); a < f.args.end(); a++) {
        if (a->type.name == "QString") {
            cpp << ", const ushort*, int";
        } else if (a->type.name == "QByteArray") {
            cpp << ", const char*, int";
        } else {
            cpp << ", " << a->type.name;
        }
    }
    // If the return type is QString or QByteArray, append a pointer to the
    // variable that will be set to the argument list. Also add a setter
    // function.
    if (f.type.name == "QString") {
        cpp << ", QString*, qstring_set";
    } else if (f.type.name == "QByteArray") {
        cpp << ", QByteArray*, qbytearray_set";
    }
    cpp << ");\n";
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
        } else if (p.optional) {
            cpp << QString("    option_%3 %2_get(const %1::Private*);")
                .arg(o.name, base, p.type.name) << endl;
        } else {
            cpp << QString("    %3 %2_get(const %1::Private*);")
                .arg(o.name, base, p.type.name) << endl;
        }
        if (p.write) {
            QString t = p.type.cSetType;
            if (t == "qstring_t") {
                t = "const ushort *str, int len";
            } else if (t == "qbytearray_t") {
                t = "const char* bytes, int len";
            }
            cpp << QString("    void %2_set(%1::Private*, %3);")
                .arg(o.name, base, t) << endl;
            if (p.optional) {
                cpp << QString("    void %2_set_none(%1::Private*);")
                    .arg(o.name, base) << endl;
            }
        }
    }

    for (const Function& f: o.functions) {
        writeFunctionCDecl(cpp, f, lcname, o);
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
        cpp << "}\n";
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
        } else if (p.optional) {
            cpp << QString("QVariant %1::%2() const\n{\n").arg(o.name, p.name);
            cpp << "    QVariant v;\n";
            cpp << QString("    auto r = %1_get(m_d);\n").arg(base);
            cpp << "    if (r.some) {\n";
            cpp << "        v.setValue(r.value);\n";
            cpp << "    }\n";
            cpp << "    return r;\n";
            cpp << "}\n";
        } else {
            cpp << QString("%3 %1::%2() const\n{\n").arg(o.name, p.name, p.type.name);
            cpp << QString("    return %1_get(m_d);\n}\n").arg(base);
        }
        if (p.write) {
            auto t = p.type.cppSetType;
            if (p.optional && !p.type.isComplex()) {
                t = "const QVariant&";
            }
            cpp << "void " << o.name << "::set" << upperInitial(p.name) << "(" << t << " v) {" << endl;
            if (p.optional) {
                if (p.type.isComplex()) {
                    cpp << "    if (v.isNull()) {" << endl;
                } else {
                    cpp << QString("    if (v.isNull() || !v.canConvert<%1>()) {").arg(p.type.name) << endl;
                }
                cpp << QString("        %1_set_none(m_d);").arg(base) << endl;
                cpp << QString("    } else {") << endl;
                if (p.type.name == "QString") {
                    cpp << QString("    %1_set(m_d, reinterpret_cast<const ushort*>(v.data()), v.size());").arg(base) << endl;
                } else if (p.type.name == "QByteArray") {
                    cpp << QString("    %1_set(m_d, v.data(), v.size());").arg(base) << endl;
                } else if (p.optional) {
                    cpp << QString("        %1_set(m_d, v.value<%2>());").arg(base, p.type.name) << endl;
                } else {
                    cpp << QString("        %1_set(m_d, v);").arg(base) << endl;
                }
                cpp << QString("    }") << endl;
            } else if (p.type.name == "QString") {
                cpp << QString("    %1_set(m_d, reinterpret_cast<const ushort*>(v.data()), v.size());").arg(base) << endl;
            } else if (p.type.name == "QByteArray") {
                cpp << QString("    %1_set(m_d, v.data(), v.size());").arg(base) << endl;
            } else {
                cpp << QString("    %1_set(m_d, v);").arg(base) << endl;
            }
            cpp << "}" << endl;
        }
    }

    for (const Function& f: o.functions) {
        const QString base = QString("%1_%2")
            .arg(lcname, snakeCase(f.name));
        cpp << QString("%1 %2::%3(").arg(f.type.name, o.name, f.name);
        for (auto a = f.args.begin(); a < f.args.end(); a++) {
            cpp << QString("%1 %2%3").arg(a->type.cppSetType, a->name, a + 1 < f.args.end() ? ", " : "");
        }
        cpp << QString(")%1\n{\n").arg(f.mut ? "" : " const");
        QString argList;
        for (auto a = f.args.begin(); a < f.args.end(); a++) {
            if (a->type.name == "QString") {
                argList.append(QString(", %1.utf16(), %1.size()").arg(a->name));
            } else if (a->type.name == "QByteArray") {
                argList.append(QString(", %1.data(), %1.size()").arg(a->name));
            } else {
                argList.append(QString(", %1").arg(a->name));
            }
        }
        if (f.type.name == "QString") {
            cpp << QString("    %1 s;").arg(f.type.name) << endl;
            cpp << QString("    %1(m_d%2, &s, set_qstring);")
                .arg(base, argList) << endl;
            cpp << "    return s;" << endl;
        } else if (f.type.name == "QByteArray") {
            cpp << QString("    %1 s;").arg(f.type.name) << endl;
            cpp << QString("    %1(m_d%2, &s, set_qbytearray);")
                .arg(base, argList) << endl;
            cpp << "    return s;" << endl;
        } else {
            cpp << QString("    return %1(m_d%2);")
                .arg(base, argList) << endl;
        }
        cpp << "}" << endl;
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
)").arg(conf.hFile.fileName());
    for (auto option: conf.optionalTypes()) {
        if (option != "QString" && option != "QByteArray") {
            cpp << QString(R"(
    struct option_%1 {
    public:
        %1 value;
        bool some;
        operator QVariant() const {
            if (some) {
                return QVariant::fromValue(value);
            }
            return QVariant();
        }
    };
    static_assert(std::is_pod<option_%1>::value, "option_%1 must be a POD type.");
)").arg(option);
        }
    }
    if (conf.types().contains("QString")) {
        cpp << R"(
    typedef void (*qstring_set)(QString* val, const char* utf8, int nbytes);
    void set_qstring(QString* val, const char* utf8, int nbytes) {
        *val = QString::fromUtf8(utf8, nbytes);
    }
)";
    }
    if (conf.types().contains("QByteArray")) {
        cpp << R"(
    typedef void (*qbytearray_set)(QByteArray* val, const char* bytes, int nbytes);
    void set_qbytearray(QByteArray* v, const char* bytes, int nbytes) {
        if (v->isNull() && nbytes == 0) {
            *v = QByteArray(bytes, nbytes);
        } else {
            v->truncate(0);
            v->append(bytes, nbytes);
        }
    }
)";
    }
    if (conf.hasListOrTree()) {
        cpp << R"(
    struct qmodelindex_t {
        int row;
        quintptr id;
    };
    inline QVariant cleanNullQVariant(const QVariant& v) {
        return (v.isNull()) ?QVariant() :v;
    }
)";
    }

    for (auto o: conf.objects) {
        for (auto p: o.properties) {
            if (p.type.type == BindingType::Object) {
                continue;
            }
            cpp << "    inline void " << changedF(o, p) << "(" << o.name << "* o)\n";
            cpp << "    {\n        emit o->" << p.name << "Changed();\n    }\n";
        }
    }
    cpp << "}\n";

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
