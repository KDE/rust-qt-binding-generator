#include "Tree.h"
#include "Fibonacci.h"
#include <cstdlib>

#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>
#include <QApplication>
#include <QCommandLineParser>
#include <QTreeView>
#include <QHeaderView>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QQmlContext>
#include <QSortFilterProxyModel>

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

    qmlRegisterType<QSortFilterProxyModel>("org.qtproject.example", 1, 0, "SortFilterProxyModel");
    qmlRegisterType<Fibonacci>("rust", 1, 0, "Fibonacci");
    qmlRegisterType<FibonacciList>("rust", 1, 0, "FibonacciList");

    Tree model;
    model.setPath("/");
    QSortFilterProxyModel sortedModel;
    sortedModel.setSourceModel(&model);
    sortedModel.setDynamicSortFilter(true);
    QTreeView view;
    view.setUniformRowHeights(true);
    view.setSortingEnabled(true);
    view.setModel(&sortedModel);
    auto root = sortedModel.index(0, 0);
    view.expand(root);
    view.sortByColumn(0, Qt::AscendingOrder);
    view.show();
    view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("fsModel", &model);
    engine.rootContext()->setContextProperty("sortedFsModel", &sortedModel);
    engine.load(QUrl(QStringLiteral("qrc:///demo.qml")));
    return app.exec();
}
