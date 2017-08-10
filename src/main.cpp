#include "RMailObject.h"
#include "tmp.h"
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
#include <QDebug>

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("RMail");
    
    KAboutData aboutData(
                         // The program name used internally. (componentName)
                         QStringLiteral("RMail"),
                         // A displayable program name string. (displayName)
                         i18n("RMail"),
                         // The program version string. (version)
                         QStringLiteral("0.1"),
                         // Short description of what the app does. (shortDescription)
                         i18n("Displays a mails from a maildir"),
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

    QQmlApplicationEngine engine;

    RItemModel model;
    QTreeView view;
    view.setUniformRowHeights(true);
    view.setModel(&model);
    view.show();

    engine.rootContext()->setContextProperty("fsModel", &model);
    engine.load(QUrl(QStringLiteral("test.qml")));

    return app.exec();
/*
    RMailObject rmail;
    rmail.setUserName("RMail");
    rmail.userName();

    KGuiItem yesButton( rmail.userName(), rmail.userName(),
                        i18n( "This is a tooltip" ),
                        i18n( "This is a WhatsThis help text." ) );

    return 
        KMessageBox::questionYesNo 
        (0, i18n( "Hello World" ), i18n( "Hello" ), yesButton ) 
        == KMessageBox::Yes? EXIT_SUCCESS: EXIT_FAILURE;
*/
}
