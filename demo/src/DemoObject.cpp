#include "DemoObject.h"
#include <QSize>
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
        int column;
        uint64_t id;
        qmodelindex_t(const QModelIndex& m):
           row(m.row()), column(m.column()), id(m.internalId()) {}
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
    int ritemmodel_column_count(RItemModelInterface*, qmodelindex_t parent);
    int ritemmodel_row_count(RItemModelInterface*, qmodelindex_t parent);
    qmodelindex_t ritemmodel_index(RItemModelInterface*, int row, int column, qmodelindex_t parent);
    qmodelindex_t ritemmodel_parent(RItemModelInterface*, qmodelindex_t);
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
}

RItemModel::~RItemModel() {
    ritemmodel_free(d);
}

void RItemModel::handleNewData() {
    qDebug() << "new data!";
}

int RItemModel::columnCount(const QModelIndex &parent) const
{
    return ritemmodel_column_count(d, parent);
}
int RItemModel::rowCount(const QModelIndex &parent) const
{
    return ritemmodel_row_count(d, parent);
}

QVariant RItemModel::data(const QModelIndex &index, int role) const
{
    QVariant v;
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            QString s;
            ritemmodel_data_file_name(d, index.row(), index.internalId(), &s, set_qstring);
            v = s;
        } else if (index.column() == 1) {
            v.setValue<int>(ritemmodel_data_file_permissions(d, index.row(), index.internalId()));
        }
    }
    return v;
}
QModelIndex RItemModel::index(int row, int column, const QModelIndex &parent) const
{
    const qmodelindex_t i = ritemmodel_index(d, row, column, parent);
    return i.id ?createIndex(i.row, i.column, i.id) :QModelIndex();
}
QModelIndex RItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const qmodelindex_t parent = ritemmodel_parent(d, index);
    return parent.id ?createIndex(parent.row, parent.column, parent.id) :QModelIndex();
}
