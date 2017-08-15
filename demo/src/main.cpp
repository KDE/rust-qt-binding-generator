#include "Tree.h"
#include "Bindings.h"
#include <cstdlib>

#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>
#include <QApplication>
#include <QCommandLineParser>
#include <QTreeView>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QQmlContext>

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("Demo");
    
    KAboutData aboutData(
                         // The program name used internally. (componentName)
                         QStringLiteral("Demo"),
                         // A displayable program name string. (displayName)
                         i18n("Demo"),
                         // The program version string. (version)
                         QStringLiteral("0.1"),
                         // Short description of what the app does. (shortDescription)
                         i18n("Demo application for Rust bindings"),
                         // The license this code is released under
                         KAboutLicense::GPL,
                         // Copyright Statement (copyrightStatement = QString())
                         i18n("(c) 2017"),
                         // Optional text shown in the About box.
                         // Can contain any information desired. (otherText)
                         i18n("Some text..."),
                         // The program homepage string. (homePageAddress = QString())
                         QStringLiteral("http://kde.org/"),
                         // The bug report email address
                         // (bugsEmailAddress = QLatin1String("submit@bugs.kde.org")
                         QStringLiteral("submit@bugs.kde.org"));
    aboutData.addAuthor(i18n("Jos van den Oever"), i18n("Task"), QStringLiteral("your@email.com"),
                         QStringLiteral("http://vandenoever.info"), QStringLiteral("OSC Username"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    qmlRegisterType<Directory>("rust", 1, 0, "Directory");
    qmlRegisterType<Person>("rust", 1, 0, "Person");

    Tree model;
    model.setPath("/");
    QTreeView view;
    view.setUniformRowHeights(true);
    view.setModel(&model);
    view.show();
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("fsModel", &model);
    engine.load(QUrl(QStringLiteral("qrc:///demo.qml")));
    return app.exec();
}
