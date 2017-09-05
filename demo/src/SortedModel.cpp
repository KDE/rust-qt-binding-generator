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

#include "SortedModel.h"

bool SortedModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent)) {
        return true;
    }

    QModelIndex source_index = sourceModel()->index(source_row, 0, source_parent);
    for (int i = 0 ; i < sourceModel()->rowCount(source_index); ++i) {
        if (filterAcceptsRow(i, source_index)) return true;
    }

    return false;
}
