import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

Item {
    TextField {
        id: processFilterInput
        focus: true
        width: parent.width
        placeholderText: "Filter processes"
        onTextChanged: {
            processes.filterRegExp = new RegExp(processFilterInput.text)
        }
    }
    TreeView {
        onClicked: {
            processSelection.select(
                        index, ItemSelectionModel.ToggleCurrent)
        }
        Binding {
            target: demo.processes
            property: "active"
            value: visible
        }
        width: parent.width
        anchors.top: processFilterInput.bottom
        anchors.bottom: parent.bottom
        id: processView
        model: processes
        selection: processSelection
        selectionMode: SelectionMode.ExtendedSelection
        //                    selectionMode: SelectionMode.SingleSelection
        sortIndicatorVisible: true
        alternatingRowColors: true
        TableViewColumn {
            title: "name"
            role: "name"
        }
        TableViewColumn {
            title: "cpu"
            role: "cpuUsage"
        }
        TableViewColumn {
            title: "memory"
            role: "memory"
        }
        onSortIndicatorColumnChanged: sort()
        onSortIndicatorOrderChanged: sort()
        function sort() {
            var role = getColumn(
                        processView.sortIndicatorColumn).role
            model.sortByRole(role, processView.sortIndicatorOrder)
        }
        Timer {
            interval: 100
            running: true
            repeat: true
            onTriggered: {
                var root = processView.rootIndex
                var systemd = processes.index(1, 0, root)
                if (processes.data(systemd) === "systemd") {
                    processView.expand(systemd)
                    running = false
                }
            }
        }
    }
}
