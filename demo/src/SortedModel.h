#ifndef SORTED_MODEL
#define SORTED_MODEL

#include <QSortFilterProxyModel>
#include <QDebug>

class SortedModel : public QSortFilterProxyModel {
Q_OBJECT
public:
    SortedModel() :QSortFilterProxyModel() {}
public slots:
    void sortByRole(const QString& role, Qt::SortOrder order) {
        QHashIterator<int, QByteArray> i(roleNames());
        while (i.hasNext()) {
            i.next();
            if (i.value() == role) {
                setSortRole(i.key());
            }
        }
        sort(0, order);
    }
};

#endif
