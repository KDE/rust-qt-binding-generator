#include "Bindings.h"

#include <QtCore/QFile>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/qqml.h>

extern "C" {
    int main_cpp(const char* app);
}

int main_cpp(const char* appPath)
{
    int argc = 1;
    char* argv[1] = { (char*)appPath };
    QGuiApplication app(argc, argv);
    qmlRegisterType<Simple>("RustCode", 1, 0, "Simple");

    QQmlApplicationEngine engine;
    if (QFile("main.qml").exists()) {
        engine.load(QUrl(QStringLiteral("main.qml")));
    } else {
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    }
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
