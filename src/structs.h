#include <QString>
#include <QByteArray>
#include <QList>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>

enum class ObjectType {
    Object,
    List,
    UniformTree
};

enum class BindingType {
    Bool,
    UChar,
    Int,
    UInt,
    ULongLong,
    Float,
    QString,
    QByteArray,
    Object,
};

struct BindingTypeProperties {
    BindingType type;
    QString name;
    QString cppSetType;
    QString cSetType;
    QString rustType;
    QString rustTypeInit;
    bool isComplex() const {
        return name.startsWith("Q");
    }
};

struct Property {
    QString name;
    BindingTypeProperties type;
    bool write;
    bool optional;
};

struct ItemProperty {
    QString name;
    BindingTypeProperties type;
    bool write;
    bool optional;
    QList<QList<Qt::ItemDataRole>> roles;
};

struct Object {
    QString name;
    ObjectType type;
    QList<Property> properties;
    QList<ItemProperty> itemProperties;
    int columnCount;
    bool containsObject() {
        for (auto p: properties) {
            if (p.type.type == BindingType::Object) {
                return true;
            }
        }
        return false;
    }
};

struct Configuration {
    QFileInfo hFile;
    QFileInfo cppFile;
    QDir rustdir;
    QString interfaceModule;
    QString implementationModule;
    QList<Object> objects;
    bool overwriteImplementation;
    const Object& findObject(const QString& name) const {
        for (auto& o: objects) {
            if (o.name == name) {
                return o;
            }
        }
        QTextStream err(stderr);
        err << QCoreApplication::translate("main",
            "Cannot find type %1.\n").arg(name);
        err.flush();
        exit(1);
    }
};

