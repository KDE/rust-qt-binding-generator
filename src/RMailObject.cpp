#include "RMailObject.h"
#include <cstdint>
#include <unistd.h>

extern "C" {
    RMailObjectInterface* hello_new(void*, void (*)(RMailObject*));
    void hello_free(RMailObjectInterface*);
    void hello_set(RMailObjectInterface*, const uint16_t *, size_t);
    size_t hello_size(RMailObjectInterface*);
    const char* hello_get(RMailObjectInterface*);

    RItemModelInterface* ritemmodel_new(void*);
    void ritemmodel_free(RItemModelInterface*);
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
    return QString::fromUtf8(hello_get(d), hello_size(d));
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
    return 0;
}
QVariant RItemModel::data(const QModelIndex &index, int role) const
{
    return 0;
}
QModelIndex RItemModel::index(int row, int column, const QModelIndex &parent) const
{
    return QModelIndex();
}
QModelIndex RItemModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}
int RItemModel::rowCount(const QModelIndex &parent) const
{
    return 0;
}
