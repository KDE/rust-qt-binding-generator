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

RowLayout {
    Component {
        id: editableDelegate
        Item {
            function round(v) {
                return Math.round(1000 * v) / 1000
            }
            Text {
                text: round(styleData.value)
                visible: !styleData.selected
            }
            Loader {
                id: loaderEditor
                anchors.fill: parent
                Connections {
                    target: loaderEditor.item
                    onEditingFinished: {
                        if (!demo) {
                            return
                        }
                        var val = loaderEditor.item.text
                        var row = styleData.row
                        if (styleData.column === 0) {
                            demo.timeSeries.setTime(row, val)
                        } else if (styleData.column === 1) {
                            demo.timeSeries.setSin(row, val)
                        } else {
                            demo.timeSeries.setCos(row, val)
                        }
                    }
                }
                sourceComponent: styleData.selected ? editor : null
            }
            Component {
                id: editor
                TextInput {
                    id: textInput
                    text: round(styleData.value)
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: textInput.forceActiveFocus()
                    }
                }
            }
        }
    }
    SplitView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        handleDelegate: Rectangle {
            width: 3
        }
        TableView {
            model: demo.timeSeries
            Layout.fillHeight: true

            TableViewColumn {
                role: "time"
                title: qsTr("time")
            }
            TableViewColumn {
                role: "sin"
                title: qsTr("sin")
            }
            TableViewColumn {
                role: "cos"
                title: qsTr("cos")
            }
            itemDelegate: {
                return editableDelegate
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Text {
                anchors.centerIn: parent
                text: qsTr("QtChart is not available.")
                visible: chartLoader.status !== Loader.Ready
            }
            Loader {
                anchors.fill: parent
                id: chartLoader
                source: "chart.qml"
            }
        }
    }
}
