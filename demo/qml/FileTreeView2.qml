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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ListView {
    id: view
    property string title
    header: Column {
        width: parent.width
        ToolBar {
            width: parent.width
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    text: qsTr("‹")
                    enabled: dirModel.rootIndex.valid
                    onClicked: {
                        dirModel.rootIndex = dirModel.rootIndex.parent
                    }
                }
                Label {
                    text: view.title
                    elide: Label.ElideMiddle
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                }
            }
        }
        Row {
            Text {
                width: 200
                text: qsTr("Name")
            }
            Text {
                text: qsTr("Size")
            }
        }
    }
    model: DelegateModel {
        id: dirModel
        model: sortedFileSystem
        onRootIndexChanged: {
            var index = sortedFileSystem.mapToSource(rootIndex);
            view.title = demo.fileSystemTree.filePath(index) || "";
        }
        delegate: Item {
            width: parent.width
            height: row.height
            Row {
                id: row
                Connections {
                    target: sortedFileSystem
                    onRowsInserted: {
                        // enable the button if children were found when 'model'
                        // was created or if they were just inserted
                        button.enabled = model.hasModelChildren
                                || dirModel.modelIndex(index) === parent
                    }
                }
                Button {
                    id: button
                    width: 200
                    text: fileName
                    enabled: model.hasModelChildren
                    onClicked: {
                        view.model.rootIndex = view.model.modelIndex(index)
                    }
                }
                Label {
                    text: fileSize
                    padding: button.padding
                }
            }
        }
    }
}
