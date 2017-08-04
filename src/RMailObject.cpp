#include "RMailObject.h"
#include <QSize>
#include <QDebug>
#include <cstdint>
#include <unistd.h>

namespace {
    typedef struct {
    private:
        const char* data;
        int len;
    public:
        operator QByteArray() const {
            return QByteArray(data, len);
        }
    } qbytearray_t;
    struct qstring_t {
    private:
        const char* data;
        int len;
    public:
        operator QString() const {
            return QString::fromUtf8(data, len);
        }
    };
    typedef struct {
        int row;
        int column;
        uint64_t id;
    } qmodelindex_t;
    typedef struct qvariant_t {
        unsigned int type;
        int value;
        const char* data;
    } qvariant_t;
    QVariant variant(const qvariant_t& v) {
        switch (v.type) {
            case QVariant::Bool: return QVariant((bool)v.value);
            case QVariant::String: return QString::fromUtf8(v.data, v.value);
            default:;
        }
        return QVariant();
    }

/*
    qvariant_t variant(const QVariant& v) {
        auto t = v.type();
        switch (t) {
            case QVariant::Bool:
                return { .type = t, .value = { .vbool = 0 } };
            case QVariant::ByteArray:
                return { .type = t, .value = { .vbool = 0 } };
            case QVariant::String:
                return { .type = t, .value = { .vbool = 0 } };
            default:;
        }
        return { .type = QVariant::Invalid, .value = { .vbool = false } };
    }
*/
}

typedef void (*qvariant_set)(void*, qvariant_t*);

extern "C" {
    RMailObjectInterface* hello_new(void*, void (*)(RMailObject*));
    void hello_free(RMailObjectInterface*);
    void hello_set(RMailObjectInterface*, const uint16_t *, size_t);
    qstring_t hello_get(RMailObjectInterface*);

    RItemModelInterface* ritemmodel_new(void*);
    void ritemmodel_free(RItemModelInterface*);
    int ritemmodel_column_count(RItemModelInterface*, qmodelindex_t parent);
    int ritemmodel_row_count(RItemModelInterface*, qmodelindex_t parent);
    qmodelindex_t ritemmodel_index(RItemModelInterface*, int row, int column, qmodelindex_t parent);
    qmodelindex_t ritemmodel_parent(RItemModelInterface*, qmodelindex_t);
    void ritemmodel_data(RItemModelInterface*, qmodelindex_t, int, void*, qvariant_set);
}

RMailObject::RMailObject(QObject *parent):
    QObject(parent),
    d(hello_new(this,
        [](RMailObject* o) { emit o->userNameChanged(); }
    ))
    {
}

RMailObject::~RMailObject() {
    hello_free(d);
}

QString
RMailObject::userName() const {
    return hello_get(d);
}

void
RMailObject::setUserName(const QString& name) {
    hello_set(d, name.utf16(), name.size());
}

const QVariantMap&
RMailObject::tree() const {
    return m_tree;
}

void
RMailObject::setTree(const QVariantMap& tree) {
    m_tree = tree;
    emit treeChanged();
}

RItemModel::RItemModel(QObject *parent):
    QAbstractItemModel(parent),
    d(ritemmodel_new(this))
{
}

RItemModel::~RItemModel() {
    ritemmodel_free(d);
}

int RItemModel::columnCount(const QModelIndex &parent) const
{
    const qmodelindex_t p = {
        .row = parent.row(),
        .column = parent.column(),
        .id = parent.internalId()
    };
    return ritemmodel_column_count(d, p);
}
int RItemModel::rowCount(const QModelIndex &parent) const
{
    const qmodelindex_t p = {
        .row = parent.row(),
        .column = parent.column(),
        .id = parent.internalId()
    };
    return ritemmodel_row_count(d, p);
}

void set_variant(void* v, qvariant_t* val) {
    *static_cast<QVariant*>(v) = variant(*val);
}

QVariant RItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    const qmodelindex_t i = {
        .row = index.row(),
        .column =  index.column(),
        .id = index.internalId()
    };
    QVariant v;
    ritemmodel_data(d, i, role, &v, set_variant);
    return v;
}
QModelIndex RItemModel::index(int row, int column, const QModelIndex &parent) const
{
    const qmodelindex_t p = {
        .row = parent.row(),
        .column =  parent.column(),
        .id = parent.internalId()
    };
    const qmodelindex_t i = ritemmodel_index(d, row, column, p);
    return i.id ?createIndex(i.row, i.column, i.id) :QModelIndex();
}
QModelIndex RItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const qmodelindex_t i = {
        .row = index.row(),
        .column =  index.column(),
        .id = index.internalId()
    };
    const qmodelindex_t parent = ritemmodel_parent(d, i);
    return parent.id ?createIndex(parent.row, parent.column, parent.id) :QModelIndex();
}
