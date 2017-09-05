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

#include <QRegExp>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

inline QString snakeCase(const QString& name) {
    return name.left(1).toLower() + name.mid(1)
        .replace(QRegExp("([A-Z])"), "_\\1").toLower();
}

// Only write a file if it is different
class DifferentFileWriter {
public:
    const QString path;
    QByteArray buffer;
    bool overwrite;
    DifferentFileWriter(const QString& p, bool o = true) :path(p), overwrite(o)
    {
    }
    ~DifferentFileWriter() {
        const QByteArray old = read();
        if (old != buffer && (old.isNull() || overwrite)) {
            write();
        }
    }
    QByteArray read() const {
        QByteArray content;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            content = file.readAll();
        }
        return content;
    }
    void write() const {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QTextStream err(stderr);
            err << QCoreApplication::translate("main",
                "Cannot write %1.\n").arg(file.fileName());
            err.flush();
            exit(1);
        }
        file.write(buffer);
        file.close();
    }
};
