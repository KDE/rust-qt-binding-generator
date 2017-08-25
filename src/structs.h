#include <QString>
#include <QByteArray>
#include <QList>
#include <QFileInfo>
#include <QDir>

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
    QByteArray
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

