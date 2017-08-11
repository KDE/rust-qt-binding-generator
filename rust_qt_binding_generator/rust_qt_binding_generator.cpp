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

enum ObjectType {
    ObjectTypeObject,
    ObjectTypeList
};

struct Property {
    QString name;
    QString type;
    bool write;
};

struct Role {
    QString name;
    QString value;
};

struct Object {
    QString name;
    ObjectType type;
    QList<Property> properties;
    QList<Role> roles;
};

struct Configuration {
    QFileInfo hFile;
    QFileInfo cppFile;
    QDir rustdir;
    QString interfaceModule;
    QString implementationModule;
    QList<Object> objects;
    bool overwriteImplementation;
};

Property
parseProperty(const QJsonObject& json) {
    Property p;
    p.name = json.value("name").toString();
    p.type = json.value("type").toString();
    p.write = json.value("write").toBool();
    return p;
}

Role
parseRole(const QJsonObject& json) {
    Role r;
    r.name = json.value("name").toString();
    r.value = json.value("value").toString();
    return r;
}

Object
parseObject(const QJsonObject& json) {
    Object o;
    o.name = json.value("name").toString();
    QString type = json.value("type").toString();
    if (type == "List") {
        o.type = ObjectTypeList;
    } else {
        o.type = ObjectTypeObject;
    }
    for (const QJsonValue& val: json.value("properties").toArray()) {
        o.properties.append(parseProperty(val.toObject()));
    }
    for (const QJsonValue& val: json.value("roles").toArray()) {
        o.roles.append(parseRole(val.toObject()));
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

QString type(const Property& p) {
    if (p.type.startsWith("Q")) {
        return "const " + p.type + "&";
    }
    return p.type;
}

QString cSetType(const Property& p) {
    if (p.type.startsWith("Q")) {
        return p.type.toLower() + "_t";
    }
    return p.type;
}

QString cGetType(const Property& p) {
    return p.type + "*, " + p.type.toLower() + "_set";
}

QString baseType(const Object& o) {
    if (o.type != ObjectTypeObject) {
        return "QAbstractItemModel";
    }
    return "QObject";
}

void writeHeaderItemModel(QTextStream& h, const Object& o) {
    h << QString(R"(
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;
    QHash<int, QByteArray> roleNames() const;
signals:
    void newDataReady();
)");
}

void writeCppListModel(QTextStream& cpp, const Object& o) {
    const QString lcname(snakeCase(o.name));

    cpp << "enum " << o.name << "Role {\n";
    for (auto role: o.roles) {
        cpp << "    " << o.name << "Role" << role.name << " = " << role.value << ",\n";
    }
    cpp << "};\n\n";

    cpp << "extern \"C\" {\n";
    for (auto role: o.roles) {
        cpp << QString("    void %2_data_%3(%1Interface*, int, QVariant*, qvariant_set);\n")
                .arg(o.name, lcname, snakeCase(role.name));
    }
    cpp << QString(R"(
    int %2_row_count(%1Interface*, qmodelindex_t parent);
}
int %1::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : 1;
}

int %1::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : %2_row_count(d, parent);
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

QVariant %1::data(const QModelIndex &index, int role) const
{
    QVariant v;
    switch ((%1Role)role) {
)").arg(o.name, lcname);

    for (auto role: o.roles) {
         cpp << QString("    case %1Role%2:\n").arg(o.name, role.name);
         cpp << QString("        %1_data_%2(d, index.row(), &v, set_qvariant);\n")
                .arg(lcname, snakeCase(role.name));
         cpp << "        break;\n";
    }
    cpp << "    }\n    return v;\n}\n";
    cpp << "QHash<int, QByteArray> " << o.name << "::roleNames() const {\n";
    cpp << "    QHash<int, QByteArray> names;\n";
    for (auto role: o.roles) {
        cpp << "    names.insert(" << role.value << ", \"" << role.name << "\");\n";
    }
    cpp << "    return names;\n";
    cpp << "}";
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
                .arg(p.type, p.name,
                     p.write ? writeProperty(p.name) :"") << endl;
    }
    h << QString(R"(public:
    explicit %1(QObject *parent = nullptr);
    ~%1();
)").arg(o.name);
    for (auto p: o.properties) {
        h << "    " << p.type << " " << p.name << "() const;" << endl;
        if (p.write) {
            h << "    void set" << upperInitial(p.name) << "(" << type(p) << " v);" << endl;
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
        h << "    " << p.type << " m_" << p.name << ";" << endl;
    }
    h << "};" << endl;
}

void writeObjectCDecl(QTextStream& cpp, const Object& o) {
    const QString lcname(snakeCase(o.name));
    cpp << QString("    %1Interface* %2_new(%1*").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        cpp << QString(", void (*)(%1*)").arg(o.name);
    }
    if (o.type == ObjectTypeList) {
        cpp << QString(R"(,
        void (*)(%1*, int, int),
        void (*)(%1*),
        void (*)(%1*, int, int),
        void (*)(%1*))").arg(o.name);
    }
    cpp << ");" << endl;
    cpp << QString("    void %2_free(%1Interface*);").arg(o.name, lcname)
        << endl;
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        if (p.type.startsWith("Q")) {
            cpp << QString("    void %2_get(%1Interface*, %3);")
                .arg(o.name, base, cGetType(p)) << endl;
        } else {
            cpp << QString("    %3 %2_get(%1Interface*);")
                .arg(o.name, base, p.type) << endl;
        }
        if (p.write) {
            cpp << QString("    void %1_set(void*, %2);")
                .arg(base, cSetType(p)) << endl;
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
    if (o.type == ObjectTypeList) {
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
    cpp << QString(R"()) {}

%1::~%1() {
    %2_free(d);
}
)").arg(o.name, lcname);

    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        cpp << QString("%3 %1::%2() const\n{\n").arg(o.name, p.name, p.type);
        if (p.type.startsWith("Q")) {
            cpp << "    " << p.type << " v;\n";
            cpp << "    " << base << "_get(d, &v, set_" << p.type.toLower()
                << ");\n";
            cpp << "    return v;\n}\n";
        } else {
            cpp << QString("    return %1_get(d);\n}\n").arg(base);
        }
        if (p.write) {
            cpp << "void " << o.name << "::set" << upperInitial(p.name) << "(" << type(p) << " v) {" << endl;
            if (p.type == "QVariant") {
                cpp << QString("    variant(v, d, %1_set);").arg(base) << endl;
            } else {
                cpp << QString("    %1_set(d, v);").arg(base) << endl;
            }
            cpp << "}" << endl;
        }
    }
    if (o.type == ObjectTypeList) {
        writeCppListModel(cpp, o);
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
#include <QVariant>
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
#include <QModelIndex>

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
        int column;
        uint64_t id;
        qmodelindex_t(const QModelIndex& m):
           row(m.row()), column(m.column()), id(m.internalId()) {}
    };
    struct qvariant_t {
        unsigned int type;
        int value;
        const char* data;
    };
    QVariant variant(const qvariant_t& v) {
        switch (v.type) {
            case QVariant::String: return QString::fromUtf8(static_cast<const char*>(v.data), v.value);
            case QVariant::Bool: return QVariant((bool)v.value);
            case QVariant::ByteArray: return QVariant(QByteArray(v.data, v.value));
            default:;
        }
        return QVariant();
    }
    void variant(const QByteArray& v, void* d, void (*set)(void*, qvariant_t)) {
        set(d, {
            .type = QVariant::ByteArray,
            .value = v.length(),
            .data = v.data()
        });
    }
    void variant(const QString& v, void* d, void (*set)(void*, qvariant_t)) {
        set(d, {
            .type = QVariant::String,
            .value = v.size(),
            .data = static_cast<const char*>(static_cast<const void*>(v.utf16()))
        });
    }
    void variant(const QVariant& v, void* d, void (*set)(void*, qvariant_t)) {
        switch (v.type()) {
            case QVariant::Bool:
                set(d, {
                    .type = QVariant::Bool,
                    .value = v.toBool(),
                    .data = 0
                });
                break;
            case QVariant::Int:
                set(d, {
                    .type = QVariant::Int,
                    .value = v.toInt(),
                    .data = 0
                });
                break;
            case QVariant::ByteArray:
                variant(v.toByteArray(), d, set);
                break;
            case QVariant::String:
                variant(v.toString(), d, set);
                break;
            default:
                set(d, {
                    .type = QVariant::Invalid,
                    .value = 0,
                    .data = 0
                });
        }
    }
}
typedef void (*qstring_set)(QString*, qstring_t*);
void set_qstring(QString* v, qstring_t* val) {
    *v = *val;
}
typedef void (*qbytearray_set)(QByteArray*, qbytearray_t*);
void set_qbytearray(QByteArray* v, qbytearray_t* val) {
    *v = *val;
}
typedef void (*qvariant_set)(QVariant*, qvariant_t*);
void set_qvariant(QVariant* v, qvariant_t* val) {
    *v = variant(*val);
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

QString rustType(const Property& p) {
    if (p.type == "QByteArray") {
        return "Vec<u8>";
    }
    if (p.type.startsWith("Q")) {
        return p.type.mid(1);
    }
    if (p.type == "int") {
        return "c_int";
    }
    return p.type;
}

QString rustTypeInit(const Property& p) {
    if (p.type == "QByteArray") {
        return "Vec::new()";
    }
    if (p.type == "QString") {
        return "String::new()";
    }
    if (p.type == "QVariant") {
        return "Variant::None";
    }
    if (p.type == "bool") {
        return "true";
    }
    return "0";
}

void writeRustInterfaceObject(QTextStream& r, const Object& o) {
    const QString lcname(snakeCase(o.name));
    r << QString(R"(/* generated by rust_qt_binding_generator */
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
    if (o.type == ObjectTypeList) {
        modelStruct = ", model: " + o.name + "List";
        r << QString(R"(}

pub struct %1List {
    qobject: *const %1QObject,
    %2_begin_insert_rows: fn(*const %1QObject, c_int, c_int),
    %2_end_insert_rows: fn(*const %1QObject),
    %2_begin_remove_rows: fn(*const %1QObject, c_int, c_int),
    %2_end_remove_rows: fn(*const %1QObject),
}

impl %1List {
    pub fn %2_begin_insert_rows(&self, first: c_int, last: c_int) {
        (self.%2_begin_insert_rows)(self.qobject, first, last);
    }
    pub fn %2_end_insert_rows(&self) {
        (self.%2_end_insert_rows)(self.qobject);
    }
    pub fn %2_begin_remove_rows(&self, first: c_int, last: c_int) {
        (self.%2_begin_remove_rows)(self.qobject, first, last);
    }
    pub fn %2_end_remove_rows(&self) {
        (self.%2_end_remove_rows)(self.qobject);
    }
)").arg(o.name, lcname);
    }

    r << QString(R"(}

pub trait %1Trait {
    fn create(emit: %1Emitter%2) -> Self;
    fn emit(&self) -> &%1Emitter;
)").arg(o.name, modelStruct);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    fn get_%1(&self) -> %2;\n").arg(lc, rustType(p));
        if (p.write) {
            r << QString("    fn set_%1(&mut self, value: %2);\n").arg(lc, rustType(p));
        }
    }
    if (o.type == ObjectTypeList) {
        r << "    fn row_count(&self) -> c_int;\n";
        for (auto role: o.roles) {
            r << QString("    fn %1(&self, row: c_int) -> Variant;\n")
                    .arg(snakeCase(role.name));
        }
    }

    r << QString(R"(}

#[no_mangle]
pub extern "C" fn %2_new(qobject: *const %1QObject)").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        r << QString(",\n        %2_changed: fn(*const %1QObject)")
            .arg(o.name, snakeCase(p.name));
    }
    if (o.type == ObjectTypeList) {
        r << QString(R"(,
        %2_begin_insert_rows: fn(*const %1QObject,
            c_int,
            c_int),
        %2_end_insert_rows: fn(*const %1QObject),
        %2_begin_remove_rows: fn(*const %1QObject,
            c_int,
            c_int),
        %2_end_remove_rows: fn(*const %1QObject))").arg(o.name, lcname);
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
    if (o.type == ObjectTypeList) {
        model = ", model";
        r << QString(R"(    };
    let model = %1List {
        qobject: qobject,
        %2_begin_insert_rows: %2_begin_insert_rows,
        %2_end_insert_rows: %2_end_insert_rows,
        %2_begin_remove_rows: %2_begin_remove_rows,
        %2_end_remove_rows: %2_end_remove_rows,
)").arg(o.name, lcname);
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
        QString ret = ") -> " + rustType(p);
        if (p.type.startsWith("Q")) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1,
        p: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).get_%3();
    set(p, %4::from(&data));
}
)").arg(o.name, base, snakeCase(p.name), p.type);
            if (p.write) {
                const QString type = p.type == "QString" ? "QStringIn" : p.type;
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
)").arg(o.name, base, snakeCase(p.name), rustType(p));
            if (p.write) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v);
}
)").arg(o.name, base, snakeCase(p.name), rustType(p));
            }
        }
    }
    if (o.type == ObjectTypeList) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_row_count(ptr: *const %1) -> c_int {
    (&*ptr).row_count()
}
)").arg(o.name, lcname);
        for (auto role: o.roles) {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_data_%3(ptr: *const %1,
                                         row: c_int,
                                         d: *mut c_void,
                                         set: fn(*mut c_void, &QVariant)) {
    let data = (& *ptr).%3(row);
    set(d, &QVariant::from(&data));
}
)").arg(o.name, lcname, snakeCase(role.name));
        }
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
    r << QString(R"(
#![allow(unknown_lints)]
#![allow(mutex_atomic, needless_pass_by_value)]
use libc::{c_int, c_void};
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
    if (o.type == ObjectTypeList) {
        modelStruct = ", model: " + o.name + "List";
        r << QString("    model: %1List,\n").arg(o.name);
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    %1: %2,\n").arg(lc, rustType(p));
    }
    r << "}\n\n";
    r << QString(R"(impl %1Trait for %1 {
    fn create(emit: %1Emitter%2) -> %1 {
        %1 {
            emit: emit,
)").arg(o.name, modelStruct);
    if (o.type == ObjectTypeList) {
        r << QString("            model: model,\n");
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("            %1: %2,\n").arg(lc, rustTypeInit(p));
    }
    r << QString(R"(        }
    }
    fn emit(&self) -> &%1Emitter {
        &self.emit
    }
)").arg(o.name);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    fn get_%1(&self) -> %2 {\n").arg(lc, rustType(p));
        if (p.type.startsWith("Q")) {
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
)").arg(lc, rustType(p));
        }
    }
    if (o.type == ObjectTypeList) {
        r << "    fn row_count(&self) -> c_int {\n        10\n    }\n";
        for (auto role: o.roles) {
            r << QString("    fn %1(&self, row: c_int) -> Variant {\n")
                    .arg(snakeCase(role.name));
            r << "        Variant::Bool(row > 0)\n";
            r << "    }\n";
        }
    }
    r << "}\n";
}

void writeRustImplementation(const Configuration& conf) {
    DifferentFileWriter w(rustFile(conf.rustdir, conf.implementationModule),
        conf.overwriteImplementation);
    QTextStream r(&w.buffer);
    r << QString(R"(use libc::c_int;
use types::*;
use %1::*;

)").arg(conf.interfaceModule);

    for (auto object: conf.objects) {
        writeRustImplementationObject(r, object);
    }
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
        QCoreApplication::translate("main", "Configuration file"));

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

    const QString configurationFile(args.at(0));
    Configuration configuration = parseConfiguration(configurationFile);
    configuration.overwriteImplementation = parser.isSet(overwriteOption);

    writeHeader(configuration);
    writeCpp(configuration);
    writeRustInterface(configuration);
    writeRustImplementation(configuration);

    return 0;
}
