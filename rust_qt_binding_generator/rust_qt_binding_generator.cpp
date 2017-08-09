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

struct Property {
    QString name;
    QString type;
    bool write;
};

struct Object {
    QString name;
    QList<Property> properties;
};

struct Configuration {
    QFileInfo hfile;
    QFileInfo cppfile;
    QDir rustdir;
    QString interfacemodule;
    QString implementationmodule;
    QList<Object> objects;
};

Property
parseProperty(const QJsonObject& json) {
    Property p;
    p.name = json.value("name").toString();
    p.type = json.value("type").toString();
    p.write = json.value("write").toBool();
    return p;
}

Object
parseObject(const QJsonObject& json) {
    Object o;
    o.name = json.value("name").toString();
    for (const QJsonValue& val: json.value("properties").toArray()) {
        o.properties.append(parseProperty(val.toObject()));
    }
    return o;
}

Configuration
parseConfiguration(const QString& path) {
    QFile configurationFile(path);
    const QDir base = QFileInfo(configurationFile).dir();
    if (!configurationFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
    c.cppfile = QFileInfo(base, o.value("cppfile").toString());
    c.hfile = QFileInfo(c.cppfile.dir(), c.cppfile.completeBaseName() + ".h");
    for (const QJsonValue& val: o.value("objects").toArray()) {
        c.objects.append(parseObject(val.toObject()));
    }
    const QJsonObject rust = o.value("rust").toObject();
    c.rustdir = QDir(base.filePath(rust.value("dir").toString()));
    c.interfacemodule = rust.value("interfacemodule").toString();
    c.implementationmodule = rust.value("implementationmodule").toString();
    return c;
}

QString upperInitial(const QString& name) {
    return name.left(1).toUpper() + name.mid(1);
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

void writeHeaderObject(QTextStream& h, const Object& o) {
    h << QString(R"(
class %1Interface;
class %1 : public QObject
{
    Q_OBJEC%2
    %1Interface * const d;
)").arg(o.name, "T");
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
    const QString lcname(o.name.toLower());
    cpp << QString("    %1Interface* %2_new(%1*").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        cpp << QString(", void (*)(%1*)").arg(o.name);
    }
    cpp << ");" << endl;
    cpp << QString("    void %2_free(%1Interface*);").arg(o.name, lcname)
        << endl;
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, p.name.toLower());
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
    const QString lcname(o.name.toLower());
    cpp << QString("%1::%1(QObject *parent):\n    QObject(parent),")
            .arg(o.name) << endl;
    cpp << QString("    d(%1_new(this").arg(lcname);
    for (const Property& p: o.properties) {
        cpp << QString(",\n        [](%1* o) { emit o->%2Changed(); }")
            .arg(o.name, p.name);
    }
    cpp << QString(R"()) {}

%1::~%1() {
    %2_free(d);
}
)").arg(o.name, lcname);

    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, p.name.toLower());
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
}

void writeHeader(const Configuration& conf) {
    QFile hFile(conf.hfile.absoluteFilePath());
    if (!hFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        err << QCoreApplication::translate("main",
            "Cannot write %1.\n").arg(hFile.fileName());
        err.flush();
        exit(1);
    }
    QTextStream h(&hFile);
    const QString guard(conf.hfile.fileName().replace('.', '_').toUpper());
    h << QString(R"(#ifndef %1
#define %1

#include <QObject>
#include <QVariant>
)").arg(guard);

    for (auto object: conf.objects) {
        writeHeaderObject(h, object);
    }

    h << QString("#endif // %1\n").arg(guard);
}

void writeCpp(const Configuration& conf) {
    QFile cppFile(conf.cppfile.absoluteFilePath());
    if (!cppFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        err << QCoreApplication::translate("main",
            "Cannot write %1.\n").arg(cppFile.fileName());
        err.flush();
        exit(1);
    }
    QTextStream cpp(&cppFile);
    cpp << QString(R"(#include "%1"
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
            case QVariant::Bool: return QVariant((bool)v.value);
            case QVariant::String: return QString::fromUtf8(static_cast<const char*>(v.data), v.value);
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
)").arg(conf.hfile.fileName());

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

void writeRustInterfaceObject(QTextStream& r, const Object& o) {
    const QString lcname(o.name.toLower());
    r << QString(R"(
pub struct %1QObject {}

#[derive (Clone)]
pub struct %1Emitter {
    qobject: Arc<Mutex<*const %1QObject>>,
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString("    %2_changed: fn(*const %1QObject),\n")
            .arg(o.name, p.name.toLower());
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
)").arg(p.name.toLower());
    }

    r << QString(R"(}

pub trait %1Trait {
    fn create(emit: %1Emitter) -> Self;
    fn emit(&self) -> &%1Emitter;
)").arg(o.name);
    for (const Property& p: o.properties) {
        const QString lc(p.name.toLower());
//        const bool q = p.type.startsWith("Q");
        r << QString("    fn get_%1(&self) -> %2;\n").arg(lc, rustType(p));
        if (p.write) {
            r << QString("    fn set_%1(&mut self, value: %2);\n").arg(lc, rustType(p));
        }
    }

    r << QString(R"(}

#[no_mangle]
pub extern "C" fn %2_new(qobject: *const %1QObject)").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        r << QString(",\n        %2_changed: fn(*const %1QObject)")
            .arg(o.name, p.name.toLower());
    }
    r << QString(R"() -> *mut %1 {
    let emit = %1Emitter {
        qobject: Arc::new(Mutex::new(qobject)))").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString(",\n        %1_changed: %1_changed").arg(p.name.toLower());
    }
    r << QString(R"(
    }; 
    let d = %1::create(emit);
    Box::into_raw(Box::new(d))
}

#[no_mangle]
pub unsafe extern "C" fn %2_free(ptr: *mut %1) {
    Box::from_raw(ptr).emit().clear();
}
)").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, p.name.toLower());
        QString ret = ") -> " + rustType(p);
        if (p.type.startsWith("Q")) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1, p: *mut c_void, set: fn (*mut c_void, %4)) {
    let data = (&*ptr).get_%3();
    set(p, %4::from(&data));
}
)").arg(o.name, base, p.name.toLower(), p.type);
            if (p.write) {
                const QString type = p.type == "QString" ? "QStringIn" : p.type;
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v.convert());
}
)").arg(o.name, base, p.name.toLower(), type);
            }
        } else {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1) -> %4 {
    (&*ptr).get_%3()
}
)").arg(o.name, base, p.name.toLower(), rustType(p));
            if (p.write) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v);
}
)").arg(o.name, base, p.name.toLower(), rustType(p));
            }
        }
    }
}

QString rustFile(const QDir rustdir, const QString& module) {
    QDir src(rustdir.absoluteFilePath("src"));
    QString path = src.absoluteFilePath(module + ".rs");
    return path;
}

void writeRustInterface(const Configuration& conf) {
    QFile file(rustFile(conf.rustdir, conf.interfacemodule));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        err << QCoreApplication::translate("main",
            "Cannot write %1.\n").arg(file.fileName());
        err.flush();
        exit(1);
    }
    QTextStream r(&file);
    r << QString(R"(
#![allow(unknown_lints)]
#![allow(mutex_atomic, needless_pass_by_value)]
use libc::{c_int, c_void};
use types::*;
use std::sync::{Arc, Mutex};
use std::ptr::null;

use %1::*;
)").arg(conf.implementationmodule);

    for (auto object: conf.objects) {
        writeRustInterfaceObject(r, object);
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

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        err << QCoreApplication::translate("main",
            "Configuration file is missing.\n");
        return 1;
    }

    const QString configurationFile(args.at(0));
    const Configuration configuration = parseConfiguration(configurationFile);

    writeHeader(configuration);
    writeCpp(configuration);
    writeRustInterface(configuration);

    return 0;
}
