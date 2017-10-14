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
    Tree
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
    Void,
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
    bool operator==(const BindingTypeProperties& other) {
        return type == other.type
            && name == other.name
            && cppSetType == other.cppSetType
            && cSetType == other.cSetType
            && rustType == other.rustType
            && rustTypeInit == other.rustTypeInit;
    }
};

struct Property {
    QString name;
    BindingTypeProperties type;
    bool write;
    bool optional;
    bool rustByValue;
};

struct Argument {
    QString name;
    BindingTypeProperties type;
};

struct Function {
    QString name;
    BindingTypeProperties type;
    QList<Argument> args;
    bool mut;
};

struct ItemProperty {
    QString name;
    BindingTypeProperties type;
    bool write;
    bool optional;
    bool rustByValue;
    QList<QList<Qt::ItemDataRole>> roles;
};

struct Object {
    QString name;
    ObjectType type;
    QList<Property> properties;
    QList<ItemProperty> itemProperties;
    QList<Function> functions;
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
    QList<QString> types() const {
        QList<QString> ops;
        for (auto o: objects) {
            for (auto ip: o.properties) {
                if (!ops.contains(ip.type.name)) {
                    ops.append(ip.type.name);
                }
            }
            for (auto ip: o.itemProperties) {
                if (!ops.contains(ip.type.name)) {
                    ops.append(ip.type.name);
                }
            }
        }
        return ops;
    }
    QList<QString> optionalTypes() const {
        QList<QString> ops;
        for (auto o: objects) {
            for (auto ip: o.itemProperties) {
                if (ip.optional && !ops.contains(ip.type.name)) {
                    ops.append(ip.type.name);
                }
            }
            if (o.type != ObjectType::Object && !ops.contains("quintptr")) {
                ops.append("quintptr");
            }
        }
        return ops;
    }
    bool hasListOrTree() const {
        for (auto o: objects) {
            if (o.type == ObjectType::List || o.type == ObjectType::Tree) {
                return true;
            }
        }
        return false;
    }
};

