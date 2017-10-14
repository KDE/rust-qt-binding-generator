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

#include "parseJson.h"
#include "helper.h"
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaEnum>

BindingTypeProperties simpleType(BindingType type, const char* name, const char* init) {
    return {
        .type = type,
        .name = name,
        .cppSetType = name,
        .cSetType = name,
        .rustType = name,
        .rustTypeInit = init
    };
}

QList<BindingTypeProperties>& bindingTypeProperties() {
    static QList<BindingTypeProperties> p;
    if (p.empty()) {
        QList<BindingTypeProperties> f;
        f.append(simpleType(BindingType::Bool, "bool", "true"));
        f.append({
            .type = BindingType::UChar,
            .name = "quint8",
            .cppSetType = "quint8",
            .cSetType = "quint8",
            .rustType = "u8",
            .rustTypeInit = "0",
        });
        f.append({
            .type = BindingType::Int,
            .name = "qint32",
            .cppSetType = "qint32",
            .cSetType = "qint32",
            .rustType = "i32",
            .rustTypeInit = "0",
        });
        f.append({
            .type = BindingType::UInt,
            .name = "quint32",
            .cppSetType = "uint",
            .cSetType = "uint",
            .rustType = "u32",
            .rustTypeInit = "0"
        });
        f.append({
            .type = BindingType::ULongLong,
            .name = "quint64",
            .cppSetType = "quint64",
            .cSetType = "quint64",
            .rustType = "u64",
            .rustTypeInit = "0"
        });
        f.append({
            .type = BindingType::Float,
            .name = "float",
            .cppSetType = "float",
            .cSetType = "float",
            .rustType = "f32",
            .rustTypeInit = "0.0"
        });
        f.append({
            .type = BindingType::QString,
            .name = "QString",
            .cppSetType = "const QString&",
            .cSetType = "qstring_t",
            .rustType = "String",
            .rustTypeInit = "String::new()"
        });
        f.append({
            .type = BindingType::QByteArray,
            .name = "QByteArray",
            .cppSetType = "const QByteArray&",
            .cSetType = "qbytearray_t",
            .rustType = "Vec<u8>",
            .rustTypeInit = "Vec::new()"
        });
        f.append({
            .type = BindingType::Void,
            .name = "void",
            .cppSetType = "void",
            .cSetType = "void",
            .rustType = "()",
            .rustTypeInit = "()",
        });
        p = f;
    }
    return p;
}

BindingTypeProperties parseBindingType(const QString& value) {
    for (auto type: bindingTypeProperties()) {
        if (value == type.name) {
            return type;
        }
    }
    QTextStream err(stderr);
    err << QCoreApplication::translate("main",
        "'%1' is not a supported type. Use one of\n").arg(value);
    for (auto i: bindingTypeProperties()) {
        if (i.name == i.rustType) {
            err << "     " << i.rustType << "\n";
        } else {
            err << "     " << i.name << " (" << i.rustType << ")\n";
        }
    }
    err.flush();
    exit(1);
}

Property
parseProperty(const QString& name, const QJsonObject& json) {
    Property p;
    p.name = name;
    p.type = parseBindingType(json.value("type").toString());
    p.write = json.value("write").toBool();
    p.optional = json.value("optional").toBool();
    p.rustByValue = json.value("rustByValue").toBool();
    return p;
}

Argument
parseArgument(const QJsonObject& json) {
    Argument arg;
    arg.name = json.value("name").toString();
    arg.type = parseBindingType(json.value("type").toString());
    QTextStream out(stdout);
    out.flush();
    if (arg.type.type == BindingType::Object) {
        QTextStream err(stderr);
        err << QCoreApplication::translate("main",
            "'%1' is not a supported type in argument \"%2\". Only use the basic QT types for now\n").arg(arg.type.name, arg.name);
        err.flush();
        exit(1);
    }
    return arg;
}

QList<Argument>
parseArguments(const QJsonArray& json) {
    QList<Argument> args;
    for(const auto& a: json) {
        args.push_back(parseArgument(a.toObject()));
    }
    return args;
}

Function
parseFunction(const QString& name, const QJsonObject& json) {
    Function f;
    f.name = name;
    f.mut = json.value("mut").toBool();
    f.type = parseBindingType(json.value("return").toString());
    if (f.type.type == BindingType::Object) {
        QTextStream err(stderr);
        err << QCoreApplication::translate("main",
            "'%1' is not a supported return type in function \"%2\". Only use the basic QT types for now\n").arg(f.type.name, f.name);
        err.flush();
        exit(1);
    }
    f.args = parseArguments(json.value("arguments").toArray());
    return f;
}

Qt::ItemDataRole parseItemDataRole(const QString& s) {
    const QString name = s.left(1).toUpper() + s.mid(1) + "Role";
    int v = QMetaEnum::fromType<Qt::ItemDataRole>()
            .keyToValue(name.toUtf8());
    if (v >= 0) {
        return (Qt::ItemDataRole)v;
    }
    QTextStream err(stderr);
    err << QCoreApplication::translate("main",
        "%1 is not a valid role name.\n").arg(s);
    err.flush();
    exit(1);
}

ItemProperty
parseItemProperty(const QString& name, const QJsonObject& json) {
    ItemProperty ip;
    ip.name = name;
    ip.type = parseBindingType(json.value("type").toString());
    ip.write = json.value("write").toBool();
    ip.optional = json.value("optional").toBool();
    ip.rustByValue = json.value("rustByValue").toBool();
    QJsonArray roles = json.value("roles").toArray();
    for (auto r: roles) {
        QList<Qt::ItemDataRole> l;
        for (auto v: r.toArray()) {
            l.append(parseItemDataRole(v.toString()));
        }
        ip.roles.append(l);
    }
    return ip;
}

Object
parseObject(const QString& name, const QJsonObject& json) {
    Object o;
    o.name = name;
    QString type = json.value("type").toString();
    if (type == "List") {
        o.type = ObjectType::List;
    } else if (type == "Tree") {
        o.type = ObjectType::Tree;
    } else {
        o.type = ObjectType::Object;
    }
    const QJsonObject& properties = json.value("properties").toObject();
    for (const QString& key: properties.keys()) {
        o.properties.append(parseProperty(key, properties[key].toObject()));
    }
    const QJsonObject& functions = json.value("functions").toObject();
    for (const QString& key: functions.keys()) {
        o.functions.append(parseFunction(key, functions[key].toObject()));
    }
    QTextStream err(stderr);
    const QJsonObject& itemProperties = json.value("itemProperties").toObject();
    if (o.type != ObjectType::Object && itemProperties.size() == 0) {
        err << QCoreApplication::translate("main",
            "No item properties are defined for %1.\n").arg(o.name);
        err.flush();
        exit(1);
    } else if (o.type == ObjectType::Object && itemProperties.size() > 0) {
        err << QCoreApplication::translate("main",
            "%1 is an Object and should not have itemProperties.").arg(o.name);
        err.flush();
        exit(1);
    }
    o.columnCount = 0;
    for (const QString& key: itemProperties.keys()) {
        ItemProperty p = parseItemProperty(key, itemProperties[key].toObject());
        o.columnCount = qMax(o.columnCount, p.roles.size());
        o.itemProperties.append(p);
    }
    return o;
}

Configuration
parseConfiguration(const QString& path) {
    QFile configurationFile(path);
    const QDir base = QFileInfo(configurationFile).dir();
    QTextStream err(stderr);
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
    const QJsonObject& object = o.value("objects").toObject();
    for (const QString& key: object.keys()) {
        bindingTypeProperties().append({
            .type = BindingType::Object,
            .name = key,
            .cppSetType = key,
            .cSetType = key,
            .rustType = key,
            .rustTypeInit = "",
        });
    }
    for (const QString& key: object.keys()) {
        Object o = parseObject(key, object[key].toObject());
        c.objects.append(o);
    }
    const QJsonObject rust = o.value("rust").toObject();
    c.rustdir = QDir(base.filePath(rust.value("dir").toString()));
    c.interfaceModule = rust.value("interfaceModule").toString();
    c.implementationModule = rust.value("implementationModule").toString();
    return c;
}
