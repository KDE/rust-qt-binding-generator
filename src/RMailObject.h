#ifndef RMAIL_OBJECT_H
#define RMAIL_OBJECT_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QAbstractItemModel>

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

class RItemModelInterface;
class RItemModel : public QAbstractItemModel {
    Q_OBJECT
    RItemModelInterface* d;
public:
    explicit RItemModel(QObject *parent = nullptr);
    ~RItemModel();

    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;

private slots:
    void handleNewData();

signals:
    void newDataReady();
};

#endif // RMAIL_OBJECT_H
