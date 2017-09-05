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
