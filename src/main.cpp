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
        QTextStream err(stderr);
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
    }

    return 0;
}
