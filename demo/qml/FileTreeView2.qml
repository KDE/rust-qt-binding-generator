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
                    elide: Label.ElideRight
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
            view.title = model.data(rootIndex)
        }
        delegate: Item {
            width: parent.width
            height: row.height
            Row {
                id: row
                Connections {
                    target: demo.fileSystemTree
                    onRowsInserted: {
                        button.enabled = (model.hasModelChildren
                            || demo.fileSystemTree.filePath(parent) === model.filePath)
                    }
                }
                Button {
                    id: button
                    width: 200
                    text: fileName
                    enabled: model.hasModelChildren
                    onClicked: {
                        if (model.hasModelChildren) {
                            view.model.rootIndex = view.model.modelIndex(index)
                        }
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
