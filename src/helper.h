#include <QRegExp>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

static QTextStream err;

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
            err << QCoreApplication::translate("main",
                "Cannot write %1.\n").arg(file.fileName());
            err.flush();
            exit(1);
        }
        file.write(buffer);
        file.close();
    }
};
