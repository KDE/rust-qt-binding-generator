import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

TreeView {
    id: treeView
    model: sortedFileSystem
    sortIndicatorVisible: true
    alternatingRowColors: true
    selection: selectionModel
    TableViewColumn {
        title: "Name"
        role: "fileName"
    }
    TableViewColumn {
        title: "Size"
        role: "fileSize"
    }
    Component.onCompleted: {
        var root = treeView.rootIndex
        var first = sortedFileSystem.index(0, 0, root)
        treeView.expand(first)
    }
    onSortIndicatorColumnChanged: sort()
    onSortIndicatorOrderChanged: sort()
    function sort() {
        var role = getColumn(treeView.sortIndicatorColumn).role
        model.sortByRole(role, treeView.sortIndicatorOrder)
    }
}
