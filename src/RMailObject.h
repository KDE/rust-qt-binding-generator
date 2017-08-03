#ifndef RMAIL_OBJECT_H
#define RMAIL_OBJECT_H

#include <QObject>
#include <QString>
#include <QVariantMap>

class RMailObjectInterface;
class RMailObject : public QObject
{
    Q_OBJECT
    RMailObjectInterface* d;
    QVariantMap m_tree;
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged FINAL)
    Q_PROPERTY(QVariantMap tree READ tree WRITE setTree NOTIFY treeChanged FINAL)

public:
    explicit RMailObject(QObject *parent = nullptr);
    ~RMailObject();

    QString userName() const;
    void setUserName(const QString &userName);
    const QVariantMap& tree() const;
    void setTree(const QVariantMap &tree);

signals:
    void userNameChanged();
    void treeChanged();

private:
    QString m_userName;
};

#endif // RMAIL_OBJECT_H
