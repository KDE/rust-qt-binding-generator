#include <QCommandLineParser>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using std::endl;

QTextStream out(stdout);
QTextStream err(stderr);

enum class ObjectType {
    Object,
    List,
    UniformTree
};

enum class BindingType {
    Bool,
    Int,
    UInt,
    ULongLong,
    QString,
    QByteArray
};

struct BindingTypeProperties {
    QString name;
    QString cppSetType;
    QString cSetType;
    QString rustType;
    QString rustTypeInit;
    bool isComplex() const {
        return name.startsWith("Q");
    }
};

BindingTypeProperties simpleType(const char* name, const char* init) {
    return {
        .name = name,
        .cppSetType = name,
        .cSetType = name,
        .rustType = name,
        .rustTypeInit = init
    };
}

const QMap<BindingType, BindingTypeProperties>& bindingTypeProperties() {
    static QMap<BindingType, BindingTypeProperties> p;
    if (p.empty()) {
        QMap<BindingType, BindingTypeProperties> f;
        f.insert(BindingType::Bool, simpleType("bool", "true"));
        f.insert(BindingType::Int, {
                     .name = "int",
                     .cppSetType = "int",
                     .cSetType = "int",
                     .rustType = "c_int",
                     .rustTypeInit = "0"
                 });
        f.insert(BindingType::UInt, {
                     .name = "uint",
                     .cppSetType = "uint",
                     .cSetType = "uint",
                     .rustType = "c_uint",
                     .rustTypeInit = "0"
                 });
        f.insert(BindingType::ULongLong, {
                     .name = "ulonglong",
                     .cppSetType = "qulonglong",
                     .cSetType = "qulonglong",
                     .rustType = "c_ulonglong",
                     .rustTypeInit = "0"
                 });
        f.insert(BindingType::QString, {
                     .name = "QString",
                     .cppSetType = "const QString&",
                     .cSetType = "qstring_t",
                     .rustType = "String",
                     .rustTypeInit = "String::new()"
                 });
        f.insert(BindingType::QByteArray, {
                     .name = "QByteArray",
                     .cppSetType = "const QByteArray&",
                     .cSetType = "qbytearray_t",
                     .rustType = "Vec<u8>",
                     .rustTypeInit = "Vec::new()"
                 });
        p = f;
    }
    return p;
}

struct Property {
    QString name;
    BindingTypeProperties type;
    bool write;
};

struct Role {
    QString name;
    QString value;
    BindingTypeProperties type;
};

struct Object {
    QString name;
    ObjectType type;
    QList<Property> properties;
    QList<QList<Role>> columnRoles;
    QList<Role> allRoles;
};

struct Configuration {
    QFileInfo hFile;
    QFileInfo cppFile;
    QDir rustdir;
    QString interfaceModule;
    QString implementationModule;
    QString typesModule;
    QList<Object> objects;
    bool overwriteImplementation;
};

BindingTypeProperties parseBindingType(const QString& value) {
    QMapIterator<BindingType, BindingTypeProperties> i(bindingTypeProperties());
    while (i.hasNext()) {
        i.next();
        if (value == i.value().name) {
            return i.value();
        }
    }
    err << QCoreApplication::translate("main",
        "'%1' is not a supported type.\n").arg(value);
    err.flush();
    exit(1);
}

Property
parseProperty(const QJsonObject& json) {
    Property p;
    p.name = json.value("name").toString();
    p.type = parseBindingType(json.value("type").toString());
    p.write = json.value("write").toBool();
    return p;
}

Role
parseRole(const QJsonObject& json) {
    Role r;
    r.name = json.value("name").toString();
    r.value = json.value("value").toString();
    r.type = parseBindingType(json.value("type").toString());
    return r;
}

QList<Role> parseRoles(const QJsonArray& roles, QList<Role>& all) {
    QList<Role> r;
    for (const QJsonValue& val: roles) {
        const Role role = parseRole(val.toObject());
        r.append(role);
        bool found = false;
        for (auto ar: all) {
            if (ar.name == role.name) {
                found = true;
                if (ar.type.name != role.type.name) {
                    err << QCoreApplication::translate("main",
                        "Role %1 has two different types: %2 and %3.\n")
                           .arg(ar.name, ar.type.name, role.type.name);
                    err.flush();
                    exit(1);
                }
            }
        }
        if (!found) {
            all.append(role);
        }
    }
    return r;
}

Object
parseObject(const QJsonObject& json) {
    Object o;
    o.name = json.value("name").toString();
    QString type = json.value("type").toString();
    if (type == "List") {
        o.type = ObjectType::List;
    } else if (type == "UniformTree") {
        o.type = ObjectType::UniformTree;
    } else {
        o.type = ObjectType::Object;
    }
    for (const QJsonValue& val: json.value("properties").toArray()) {
        o.properties.append(parseProperty(val.toObject()));
    }
    QJsonArray roles = json.value("roles").toArray();
    if (o.type != ObjectType::Object && roles.size() == 0) {
        err << QCoreApplication::translate("main",
            "No roles are defined for %1.\n").arg(o.name);
        err.flush();
        exit(1);
    }
    if (roles.at(0).isArray()) {
        for (const QJsonValue& val: roles) {
            o.columnRoles.append(parseRoles(val.toArray(), o.allRoles));
        }
    } else {
        o.columnRoles.append(parseRoles(roles, o.allRoles));
    }
    return o;
}

Configuration
parseConfiguration(const QString& path) {
    QFile configurationFile(path);
    const QDir base = QFileInfo(configurationFile).dir();
    if (!configurationFile.open(QIODevice::ReadOnly)) {
        err << QCoreApplication::translate("main",
            "Cannot read %1.\n").arg(configurationFile.fileName());
        err.flush();
        exit(1);
    }
    const QByteArray data(configurationFile.readAll());
    QJsonParseError error;
    const QJsonDocument doc(QJsonDocument::fromJson(data, &error));
    if (error.error != QJsonParseError::NoError) {
        err << error.errorString();
        err.flush();
        exit(1);
    }
    const QJsonObject o = doc.object();
    Configuration c;
    c.cppFile = QFileInfo(base, o.value("cppFile").toString());
    QDir(c.cppFile.dir()).mkpath(".");
    c.hFile = QFileInfo(c.cppFile.dir(), c.cppFile.completeBaseName() + ".h");
    for (const QJsonValue& val: o.value("objects").toArray()) {
        c.objects.append(parseObject(val.toObject()));
    }
    const QJsonObject rust = o.value("rust").toObject();
    c.rustdir = QDir(base.filePath(rust.value("dir").toString()));
    c.interfaceModule = rust.value("interfaceModule").toString();
    c.implementationModule = rust.value("implementationModule").toString();
    c.typesModule = rust.value("typesModule").toString();
    return c;
}

QString upperInitial(const QString& name) {
    return name.left(1).toUpper() + name.mid(1);
}

QString snakeCase(const QString& name) {
    return name.left(1).toLower() + name.mid(1)
        .replace(QRegExp("([A-Z])"), "_\\1").toLower();
}

QString writeProperty(const QString& name) {
    return "WRITE set" + upperInitial(name) + " ";
}

QString cGetType(const BindingTypeProperties& type) {
    return type.name + "*, " + type.name.toLower() + "_set";
}

QString baseType(const Object& o) {
    if (o.type != ObjectType::Object) {
        return "QAbstractItemModel";
    }
    return "QObject";
}

void writeHeaderItemModel(QTextStream& h, const Object&) {
    h << QString(R"(
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);
    QHash<int, QByteArray> roleNames() const;
signals:
    void newDataReady();
)");
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
    for (auto role: o.allRoles) {
        if (role.type.isComplex()) {
            cpp << QString("    void %2_data_%3(const %1Interface*%5, %4);\n")
                .arg(o.name, lcname, snakeCase(role.name), cGetType(role.type), indexDecl);
        } else {
            cpp << QString("    %4 %2_data_%3(const %1Interface*%5);\n")
                .arg(o.name, lcname, snakeCase(role.name), role.type.cppSetType, indexDecl);
        }
    }
    if (o.type == ObjectType::List) {
        cpp << QString(R"(
    int %2_row_count(const %1Interface*);
    bool %2_can_fetch_more(const %1Interface*);
    void %2_fetch_more(%1Interface*);
}
int %1::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %3;
}

int %1::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %2_row_count(d);
}

QModelIndex %1::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && column == 0) {
        return createIndex(row, 0, (quintptr)0);
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
)").arg(o.name, lcname, QString::number(o.columnRoles.size()));
    } else {
        cpp << QString(R"(
    int %2_row_count(const %1Interface*, int, quintptr);
    bool %2_can_fetch_more(const %1Interface*, int, quintptr);
    void %2_fetch_more(%1Interface*, int, quintptr);
    quintptr %2_index(const %1Interface*, int, quintptr);
    qmodelindex_t %2_parent(const %1Interface*, quintptr);
}
int %1::columnCount(const QModelIndex &) const
{
    return %3;
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
    const quintptr id = %2_index(d, parent.row(), parent.internalId());
    return id ?createIndex(row, column, id) :QModelIndex();
}

QModelIndex %1::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const qmodelindex_t parent = %2_parent(d, index.internalId());
    return parent.id ?createIndex(parent.row, 0, parent.id) :QModelIndex();
}

bool %1::canFetchMore(const QModelIndex &parent) const
{
    return %2_can_fetch_more(d, parent.row(), parent.internalId());
}

void %1::fetchMore(const QModelIndex &parent)
{
    %2_fetch_more(d, parent.row(), parent.internalId());
}
)").arg(o.name, lcname, QString::number(o.columnRoles.size()));
    }

    cpp << QString(R"(
QVariant %1::data(const QModelIndex &index, int role) const
{
    QVariant v;
    QString s;
    QByteArray b;
    switch (index.column()) {
)").arg(o.name);

    for (int col = 0; col < o.columnRoles.size(); ++col) {
        auto roles = o.columnRoles[col];
        cpp << QString("    case %1:\n").arg(col);
        cpp << QString("        switch (role) {\n");
        for (auto role: roles) {
            cpp << QString("        case %1:\n").arg(role.value);
            if (role.type.name == "QString") {
                cpp << QString("            %1_data_%2(d%4, &s, set_%3);\n")
                       .arg(lcname, snakeCase(role.name), role.type.name.toLower(), index);
                cpp << "            v.setValue<QString>(s);\n";
            } else if (role.type.name == "QByteArray") {
                cpp << QString("            %1_data_%2(d%4, &b, set_%3);\n")
                       .arg(lcname, snakeCase(role.name), role.type.name.toLower(), index);
                cpp << "            v.setValue<QByteArray>(b);\n";
            } else {
                cpp << QString("            v.setValue<%3>(%1_data_%2(d%5));\n")
                       .arg(lcname, snakeCase(role.name), role.type.cppSetType, index);
            }
            cpp << "            break;\n";
        }
        cpp << "        }\n";
        cpp << "        break;\n";
    }
    cpp << "    }\n    return v;\n}\n";
    cpp << "QHash<int, QByteArray> " << o.name << "::roleNames() const {\n";
    cpp << "    QHash<int, QByteArray> names;\n";
    for (auto role: o.allRoles) {
        cpp << "    names.insert(" << role.value << ", \"" << role.name << "\");\n";
    }
    cpp << "    return names;\n";
    cpp << "}\n\n";
}

void writeHeaderObject(QTextStream& h, const Object& o) {
    h << QString(R"(
class %1Interface;
class %1 : public %3
{
    Q_OBJEC%2
    %1Interface * const d;
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
        writeHeaderItemModel(h, o);
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
    cpp << QString("    %1Interface* %2_new(%1*").arg(o.name, lcname);
    for (int i = 0; i < o.properties.size(); ++i) {
        cpp << QString(", void (*)(%1*)").arg(o.name);
    }
    if (o.type == ObjectType::List) {
        cpp << QString(R"(,
        void (*)(%1*, int, int),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*))").arg(o.name);
    }
    if (o.type == ObjectType::UniformTree) {
        cpp << QString(R"(,
        void (*)(%1*, int, quintptr, int, int),
        void (*)(%1*),
        void (*)(%1*, int, quintptr, int, int),
        void (*)(%1*))").arg(o.name);
    }
    cpp << ");" << endl;
    cpp << QString("    void %2_free(%1Interface*);").arg(o.name, lcname)
        << endl;
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        if (p.type.isComplex()) {
            cpp << QString("    void %2_get(%1Interface*, %3);")
                .arg(o.name, base, cGetType(p.type)) << endl;
        } else {
            cpp << QString("    %3 %2_get(%1Interface*);")
                .arg(o.name, base, p.type.name) << endl;
        }
        if (p.write) {
            cpp << QString("    void %1_set(void*, %2);")
                .arg(base, p.type.cSetType) << endl;
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
        [](%1* o, int first, int last) {
            emit o->beginInsertRows(QModelIndex(), first, last);
        },
        [](%1* o) {
            emit o->endInsertRows();
        },
        [](%1* o, int first, int last) {
            emit o->beginRemoveRows(QModelIndex(), first, last);
        },
        [](%1* o) {
            emit o->endRemoveRows();
        }
)").arg(o.name);
    }
    if (o.type == ObjectType::UniformTree) {
        cpp << QString(R"(,
        [](%1* o, int row, quintptr id, int first, int last) {
            emit o->beginInsertRows(o->createIndex(row, 0, id), first, last);
        },
        [](%1* o) {
            emit o->endInsertRows();
        },
        [](%1* o, int row, quintptr id, int first, int last) {
            emit o->beginRemoveRows(o->createIndex(row, 0, id), first, last);
        },
        [](%1* o) {
            emit o->endRemoveRows();
        }
)").arg(o.name);
    }
    cpp << QString(R"()) {}

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
            cpp << QString("    %1_set(d, v);").arg(base) << endl;
            cpp << "}" << endl;
        }
    }
    if (o.type != ObjectType::Object) {
        writeCppModel(cpp, o);
    }
}

// Only write a file if it is different
class DifferentFileWriter {
public:
    const QString path;
    QByteArray buffer;
    bool overwrite;
    DifferentFileWriter(const QString& p, bool o = true) :path(p), overwrite(o)
    {
    }
    ~DifferentFileWriter() {
        const QByteArray old = read();
        if (old != buffer && (old.isNull() || overwrite)) {
            write();
        }
    }
    QByteArray read() const {
        QByteArray content;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            content = file.readAll();
        }
        return content;
    }
    void write() const {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            err << QCoreApplication::translate("main",
                "Cannot write %1.\n").arg(file.fileName());
            err.flush();
            exit(1);
        }
        file.write(buffer);
        file.close();
    }
};

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

extern "C" {
)").arg(conf.hFile.fileName());

    for (auto object: conf.objects) {
        writeObjectCDecl(cpp, object);
    }
    cpp << "};" << endl;

    for (auto object: conf.objects) {
        writeCppObject(cpp, object);
    }
}

void writeRustInterfaceObject(QTextStream& r, const Object& o) {
    const QString lcname(snakeCase(o.name));
    r << QString(R"(
pub struct %1QObject {}

#[derive (Clone)]
pub struct %1Emitter {
    qobject: Arc<Mutex<*const %1QObject>>,
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString("    %2_changed: fn(*const %1QObject),\n")
            .arg(o.name, snakeCase(p.name));
    }
    r << QString(R"(}

unsafe impl Send for %1Emitter {}

impl %1Emitter {
    fn clear(&self) {
        *self.qobject.lock().unwrap() = null();
    }
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString(R"(    pub fn %1_changed(&self) {
        let ptr = *self.qobject.lock().unwrap();
        if !ptr.is_null() {
            (self.%1_changed)(ptr);
        }
    }
)").arg(snakeCase(p.name));
    }

    QString modelStruct = "";
    if (o.type != ObjectType::Object) {
        QString type = o.type == ObjectType::List ? "List" : "UniformTree";
        modelStruct = ", model: " + o.name + type;
        QString index;
        QString indexDecl;
        if (o.type == ObjectType::UniformTree) {
            indexDecl = "row: c_int, parent: usize,";
            index = "row, parent,";
        }
        r << QString(R"(}

pub struct %1%2 {
    qobject: *const %1QObject,
    begin_insert_rows: fn(*const %1QObject,%3 c_int, c_int),
    end_insert_rows: fn(*const %1QObject),
    begin_remove_rows: fn(*const %1QObject,%3 c_int, c_int),
    end_remove_rows: fn(*const %1QObject),
}

impl %1%2 {
    pub fn begin_insert_rows(&self,%3 first: c_int, last: c_int) {
        (self.begin_insert_rows)(self.qobject,%4 first, last);
    }
    pub fn end_insert_rows(&self) {
        (self.end_insert_rows)(self.qobject);
    }
    pub fn begin_remove_rows(&self,%3 first: c_int, last: c_int) {
        (self.begin_remove_rows)(self.qobject,%4 first, last);
    }
    pub fn end_remove_rows(&self) {
        (self.end_remove_rows)(self.qobject);
    }
)").arg(o.name, type, indexDecl, index);
    }

    r << QString(R"(}

pub trait %1Trait {
    fn create(emit: %1Emitter%2) -> Self;
    fn emit(&self) -> &%1Emitter;
)").arg(o.name, modelStruct);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    fn get_%1(&self) -> %2;\n").arg(lc, p.type.rustType);
        if (p.write) {
            r << QString("    fn set_%1(&mut self, value: %2);\n").arg(lc, p.type.rustType);
        }
    }
    if (o.type != ObjectType::Object) {
        QString index;
        if (o.type == ObjectType::UniformTree) {
            index = ", row: c_int, parent: usize";
        }
        r << QString(R"(    fn row_count(&self%1) -> c_int;
    fn can_fetch_more(&self%1) -> bool { false }
    fn fetch_more(&mut self%1) {}
)").arg(index);
        if (o.type == ObjectType::List) {
            index = ", row: c_int";
        }
        for (auto role: o.allRoles) {
            r << QString("    fn %1(&self%3) -> %2;\n")
                    .arg(snakeCase(role.name), role.type.rustType, index);
        }
    }
    if (o.type == ObjectType::UniformTree) {
        r << "    fn index(&self, row: c_int, parent: usize) -> usize;\n";
        r << "    fn parent(&self, parent: usize) -> QModelIndex;\n";
    }

    r << QString(R"(}

#[no_mangle]
pub extern "C" fn %2_new(qobject: *const %1QObject)").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        r << QString(",\n        %2_changed: fn(*const %1QObject)")
            .arg(o.name, snakeCase(p.name));
    }
    if (o.type != ObjectType::Object) {
        QString indexDecl;
        if (o.type == ObjectType::UniformTree) {
            indexDecl = "row: c_int, parent: usize,";
        }
        r << QString(R"(,
        begin_insert_rows: fn(*const %1QObject,%2
            c_int,
            c_int),
        end_insert_rows: fn(*const %1QObject),
        begin_remove_rows: fn(*const %1QObject,%2
            c_int,
            c_int),
        end_remove_rows: fn(*const %1QObject))").arg(o.name, indexDecl);
    }
    r << QString(R"()
        -> *mut %1 {
    let emit = %1Emitter {
        qobject: Arc::new(Mutex::new(qobject)),
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString("        %1_changed: %1_changed,\n").arg(snakeCase(p.name));
    }
    QString model = "";
    if (o.type != ObjectType::Object) {
        const QString type = o.type == ObjectType::List ? "List" : "UniformTree";
        model = ", model";
        r << QString(R"(    };
    let model = %1%2 {
        qobject: qobject,
        begin_insert_rows: begin_insert_rows,
        end_insert_rows: end_insert_rows,
        begin_remove_rows: begin_remove_rows,
        end_remove_rows: end_remove_rows,
)").arg(o.name, type);
    }
    r << QString(R"(    };
    let d = %1::create(emit%3);
    Box::into_raw(Box::new(d))
}

#[no_mangle]
pub unsafe extern "C" fn %2_free(ptr: *mut %1) {
    Box::from_raw(ptr).emit().clear();
}
)").arg(o.name, lcname, model);
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        QString ret = ") -> " + p.type.rustType;
        if (p.type.isComplex()) {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1,
        p: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).get_%3();
    set(p, %4::from(&data));
}
)").arg(o.name, base, snakeCase(p.name), p.type.name);
            if (p.write) {
                const QString type = p.type.name == "QString" ? "QStringIn" : p.type.name;
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v.convert());
}
)").arg(o.name, base, snakeCase(p.name), type);
            }
        } else {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1) -> %4 {
    (&*ptr).get_%3()
}
)").arg(o.name, base, snakeCase(p.name), p.type.rustType);
            if (p.write) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v);
}
)").arg(o.name, base, snakeCase(p.name), p.type.rustType);
            }
        }
    }
    if (o.type != ObjectType::Object) {
        QString indexDecl;
        QString index;
        if (o.type == ObjectType::UniformTree) {
            indexDecl = ", row: c_int, parent: usize";
            index = "row, parent";
        }
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_row_count(ptr: *const %1%3) -> c_int {
    (&*ptr).row_count(%4)
}
#[no_mangle]
pub unsafe extern "C" fn %2_can_fetch_more(ptr: *const %1%3) -> bool {
    (&*ptr).can_fetch_more(%4)
}
#[no_mangle]
pub unsafe extern "C" fn %2_fetch_more(ptr: *mut %1%3) {
    (&mut *ptr).fetch_more(%4)
}
)").arg(o.name, lcname, indexDecl, index);
        if (o.type == ObjectType::UniformTree) {
            indexDecl = ", parent: usize";
            index = ", parent";
        }
        for (auto role: o.allRoles) {
            if (role.type.isComplex()) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_data_%3(ptr: *const %1,
                                    row: c_int%5,
        d: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).%3(row%6);
    set(d, %4::from(&data));
}
)").arg(o.name, lcname, snakeCase(role.name), role.type.name, indexDecl, index);
            } else {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_data_%3(ptr: *const %1, row: c_int%5) -> %4 {
    (&*ptr).%3(row%6)
}
)").arg(o.name, lcname, snakeCase(role.name), role.type.rustType, indexDecl, index);
            }
        }
    }    
    if (o.type == ObjectType::UniformTree) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_index(ptr: *const %1, row: c_int, parent: usize) -> usize {
    (&*ptr).index(row, parent)
}
#[no_mangle]
pub unsafe extern "C" fn %2_parent(ptr: *const %1, parent: usize) -> QModelIndex {
    (&*ptr).parent(parent)
}
)").arg(o.name, lcname);

    }
}

QString rustFile(const QDir rustdir, const QString& module) {
    QDir src(rustdir.absoluteFilePath("src"));
    QString path = src.absoluteFilePath(module + ".rs");
    return path;
}

void writeRustInterface(const Configuration& conf) {
    DifferentFileWriter w(rustFile(conf.rustdir, conf.interfaceModule));
    QTextStream r(&w.buffer);
    r << QString(R"(/* generated by rust_qt_binding_generator */
#![allow(unknown_lints)]
#![allow(mutex_atomic, needless_pass_by_value)]
#![allow(unused_imports)]
use libc::{c_int, c_uint, c_ulonglong, c_void};
use types::*;
use std::sync::{Arc, Mutex};
use std::ptr::null;

use %1::*;
)").arg(conf.implementationModule);

    for (auto object: conf.objects) {
        writeRustInterfaceObject(r, object);
    }
}

void writeRustImplementationObject(QTextStream& r, const Object& o) {
    const QString lcname(snakeCase(o.name));
    QString modelStruct = "";
    r << QString("pub struct %1 {\n    emit: %1Emitter,\n").arg((o.name));
    if (o.type == ObjectType::List) {
        modelStruct = ", model: " + o.name + "List";
        r << QString("    model: %1List,\n").arg(o.name);
    } else if (o.type == ObjectType::UniformTree) {
        modelStruct = ", model: " + o.name + "UniformTree";
        r << QString("    model: %1UniformTree,\n").arg(o.name);
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    %1: %2,\n").arg(lc, p.type.rustType);
    }
    r << "}\n\n";
    r << QString(R"(impl %1Trait for %1 {
    fn create(emit: %1Emitter%2) -> %1 {
        %1 {
            emit: emit,
)").arg(o.name, modelStruct);
    if (o.type != ObjectType::Object) {
        r << QString("            model: model,\n");
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("            %1: %2,\n").arg(lc, p.type.rustTypeInit);
    }
    r << QString(R"(        }
    }
    fn emit(&self) -> &%1Emitter {
        &self.emit
    }
)").arg(o.name);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    fn get_%1(&self) -> %2 {\n").arg(lc, p.type.rustType);
        if (p.type.isComplex()) {
            r << QString("        self.%1.clone()\n").arg(lc);
        } else {
            r << QString("        self.%1\n").arg(lc);
        }
        r << "    }\n";
        if (p.write) {
            r << QString(R"(    fn set_%1(&mut self, value: %2) {
        self.%1 = value;
        self.emit.%1_changed();
    }
)").arg(lc, p.type.rustType);
        }
    }
    if (o.type != ObjectType::Object) {
        QString index;
        if (o.type == ObjectType::UniformTree) {
            index = ", row: c_int, parent: usize";
        }
        r << "    fn row_count(&self" << index << ") -> c_int {\n        10\n    }\n";
        if (o.type == ObjectType::UniformTree) {
            index = ", parent: usize";
        }
        for (auto role: o.allRoles) {
            r << QString("    fn %1(&self, row: c_int%3) -> %2 {\n")
                    .arg(snakeCase(role.name), role.type.rustType, index);
            r << "        " << role.type.rustTypeInit << "\n";
            r << "    }\n";
        }
    }
    if (o.type == ObjectType::UniformTree) {
        r << R"(    fn index(&self, row: c_int, parent: usize) -> usize {
        0
    }
    fn parent(&self, parent: usize) -> QModelIndex {
        QModelIndex::create(0, 0)
    }
)";
    }
    r << "}\n";
}

void writeRustImplementation(const Configuration& conf) {
    DifferentFileWriter w(rustFile(conf.rustdir, conf.implementationModule),
        conf.overwriteImplementation);
    QTextStream r(&w.buffer);
    r << QString(R"(#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use libc::c_int;
use libc::c_uint;
use types::*;
use %1::*;

)").arg(conf.interfaceModule);

    for (auto object: conf.objects) {
        writeRustImplementationObject(r, object);
    }
}

void writeRustTypes(const Configuration& conf) {
    DifferentFileWriter w(rustFile(conf.rustdir, conf.typesModule));
    QTextStream r(&w.buffer);
    r << QString(R"(/* generated by rust_qt_binding_generator */
#![allow(dead_code)]
use std::slice;
use libc::{c_int, uint8_t, uint16_t};

#[repr(C)]
pub struct QString {
    data: *const uint8_t,
    len: c_int,
}

#[repr(C)]
pub struct QStringIn {
    data: *const uint16_t,
    len: c_int,
}

impl QStringIn {
    pub fn convert(&self) -> String {
        let data = unsafe { slice::from_raw_parts(self.data, self.len as usize) };
        String::from_utf16_lossy(data)
    }
}

impl<'a> From<&'a String> for QString {
    fn from(string: &'a String) -> QString {
        QString {
            len: string.len() as c_int,
            data: string.as_ptr(),
        }
    }
}

#[repr(C)]
pub struct QByteArray {
    data: *const uint8_t,
    len: c_int,
}

impl QByteArray {
    pub fn convert(&self) -> Vec<u8> {
        let data = unsafe { slice::from_raw_parts(self.data, self.len as usize) };
        Vec::from(data)
    }
}

impl<'a> From<&'a Vec<u8>> for QByteArray {
    fn from(value: &'a Vec<u8>) -> QByteArray {
        QByteArray {
            len: value.len() as c_int,
            data: value.as_ptr(),
        }
    }
}

#[repr(C)]
pub struct QModelIndex {
    row: c_int,
    internal_id: usize,
}

impl QModelIndex {
    pub fn invalid() -> QModelIndex {
        QModelIndex {
            row: -1,
            internal_id: 0,
        }
    }
    pub fn create(row: c_int, id: usize) -> QModelIndex {
        QModelIndex {
            row: row,
            internal_id: id,
        }
    }
}

)");
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(argv[0]);
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Generates bindings between Qt and Rust");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("configuration",
        QCoreApplication::translate("main", "Configuration file(s)"));

    // A boolean option (--overwrite-implementation)
    QCommandLineOption overwriteOption(QStringList()
            << "overwrite-implementation",
            QCoreApplication::translate("main",
                "Overwrite existing implementation."));
    parser.addOption(overwriteOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        err << QCoreApplication::translate("main",
            "Configuration file is missing.\n");
        return 1;
    }

    for (auto path: args) {
        const QString configurationFile(path);
        Configuration configuration = parseConfiguration(configurationFile);
        configuration.overwriteImplementation = parser.isSet(overwriteOption);
    
        writeHeader(configuration);
        writeCpp(configuration);
        writeRustInterface(configuration);
        writeRustImplementation(configuration);
        writeRustTypes(configuration);
    }

    return 0;
}
