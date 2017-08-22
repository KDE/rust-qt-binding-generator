#include "parseJson.h"
#include "cpp.h"
#include "rust.h"
#include "helper.h"
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(argv[0]);
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Generates bindings between Qt and Rust");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("configuration",
        QCoreApplication::translate("main", "Configuration file(s)"));

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

    for (auto path: args) {
        const QString configurationFile(path);
        Configuration configuration = parseConfiguration(configurationFile);
        configuration.overwriteImplementation = parser.isSet(overwriteOption);
    
        writeHeader(configuration);
        writeCpp(configuration);
        writeRustInterface(configuration);
        writeRustImplementation(configuration);
        writeRustTypes(configuration);
    }

    return 0;
}
