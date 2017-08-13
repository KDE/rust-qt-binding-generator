#include "DemoObject.h"
#include <QSize>
#include <QColor>
#include <QDebug>
#include <cstdint>
#include <unistd.h>

namespace {
    struct qbytearray_t {
    private:
        const char* data;
        int len;
    public:
        operator QByteArray() const {
            return QByteArray(data, len);
        }
    };
    struct qstring_t {
    private:
        const char* data;
        int len;
    public:
        operator QString() const {
            return QString::fromUtf8(data, len);
        }
    };
    struct qmodelindex_t {
        int row;
        quintptr id;
    };
}

typedef void (*qstring_set)(QString*, qstring_t*);
void set_qstring(QString* v, qstring_t* val) {
    *v = *val;
}

extern "C" {
    DemoObjectInterface* hello_new(DemoObject*, void (*)(DemoObject*));
    void hello_free(DemoObjectInterface*);
    void hello_set(DemoObjectInterface*, const uint16_t *, size_t);
    qstring_t hello_get(DemoObjectInterface*);

    RItemModelInterface* ritemmodel_new(RItemModel*, void (*)(RItemModel*));
    void ritemmodel_free(RItemModelInterface*);
    int ritemmodel_row_count(RItemModelInterface*, int, quintptr);
    quintptr ritemmodel_index(RItemModelInterface*, int, quintptr);
    qmodelindex_t ritemmodel_parent(RItemModelInterface*, int, quintptr);
    void ritemmodel_data_file_name(RItemModelInterface*, int, quintptr, QString*, qstring_set);
    int ritemmodel_data_file_permissions(RItemModelInterface*, int, quintptr);
}

DemoObject::DemoObject(QObject *parent):
    QObject(parent),
    d(hello_new(this,
        [](DemoObject* o) { emit o->userNameChanged(); }
    ))
    {
}

DemoObject::~DemoObject() {
    hello_free(d);
}

QString
DemoObject::userName() const {
    return hello_get(d);
}

void
DemoObject::setUserName(const QString& name) {
    hello_set(d, name.utf16(), name.size());
}

const QVariantMap&
DemoObject::tree() const {
    return m_tree;
}

void
DemoObject::setTree(const QVariantMap& tree) {
    m_tree = tree;
    emit treeChanged();
}

RItemModel::RItemModel(QObject *parent):
    QAbstractItemModel(parent),
    d(ritemmodel_new(this,
        [](RItemModel* o) { emit o->newDataReady(); }
    ))
{
    connect(this, &RItemModel::newDataReady, this, &RItemModel::handleNewData,
        Qt::QueuedConnection);
    qDebug() << sizeof(QObject) << sizeof(RItemModel);
    qDebug() << setHeaderData(0, Qt::Horizontal, "FileIcon", Qt::DecorationRole);
    setHeaderData(0, Qt::Horizontal, "FilePath", Qt::UserRole + 1);
    setHeaderData(0, Qt::Horizontal, "FileName", Qt::UserRole + 2);
    setHeaderData(0, Qt::Horizontal, "FilePermissions", Qt::UserRole + 3);
}

QVariant RItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        return QVariant("hello");
    }
    if (role == Qt::BackgroundRole) {
        return QVariant(QColor(Qt::green));
    }
    if (role == Qt::FontRole) {
        return QVariant("Times");
    }
    return QVariant();
}

RItemModel::~RItemModel() {
    ritemmodel_free(d);
}

void RItemModel::handleNewData() {
    qDebug() << "new data!";
}

int RItemModel::columnCount(const QModelIndex &) const
{
    return 2;
}
int RItemModel::rowCount(const QModelIndex &parent) const
{
    return ritemmodel_row_count(d, parent.row(), parent.internalId());
}

QVariant RItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::BackgroundRole) {
        return QVariant(QColor(Qt::green));
    }
    if (role == Qt::FontRole) {
        return QVariant("Times");
    }
    QVariant v;
    if (index.column() == 0) {
        if (role == Qt::DisplayRole || role == Qt::UserRole + 2) {
            QString s;
            ritemmodel_data_file_name(d, index.row(), index.internalId(), &s, set_qstring);
            v = s;
        } else if (role == Qt::UserRole + 3) {
            v.setValue<int>(ritemmodel_data_file_permissions(d, index.row(), index.internalId()));
        }
    } else if (index.column() == 1) {
        if (role == Qt::DisplayRole) {
            v.setValue<int>(ritemmodel_data_file_permissions(d, index.row(), index.internalId()));
        }
    }
    return v;
}
QModelIndex RItemModel::index(int row, int column, const QModelIndex &parent) const
{
    const quintptr id = ritemmodel_index(d, parent.row(), parent.internalId());
    return id ?createIndex(row, column, id) :QModelIndex();
}
QModelIndex RItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const qmodelindex_t parent = ritemmodel_parent(d, index.row(), index.internalId());
    return parent.id ?createIndex(parent.row, 0, parent.id) :QModelIndex();
}
QHash<int, QByteArray> RItemModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(Qt::DecorationRole, "FileIcon");
    names.insert(Qt::UserRole + 1, "FilePath");
    names.insert(Qt::UserRole + 2, "FileName");
    names.insert(Qt::UserRole + 3, "FilePermissions");
    return names;
}
