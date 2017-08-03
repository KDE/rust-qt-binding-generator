#include "RMailObject.h"
#include <cstdint>
#include <unistd.h>

extern "C" {
    RMailObjectInterface* hello_new(void*, void (*)(RMailObject*));
    void hello_set(RMailObjectInterface*, const uint16_t *, size_t);
    size_t hello_size(RMailObjectInterface*);
    const char* hello_get(RMailObjectInterface*);
    void hello_free(RMailObjectInterface*);
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
