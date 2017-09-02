import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Item {
    anchors.fill: parent

    ListView {
        id: view
        anchors.fill: parent
        header: Column {
            ToolBar {
                RowLayout {
                    anchors.fill: parent
                    ToolButton {
                        text: qsTr("‹")
                        enabled: dirModel.rootIndex.valid
                        onClicked: {
                            dirModel.rootIndex = dirModel.rootIndex.parent
                        }
                    }
                }
            } Row {
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
            delegate: Item {
                width: parent.width
                height: row.height
                Row {
                    id: row
                    Button {
                        id: button
                        width: 200
                        text: fileName
                        enabled: model.hasModelChildren
                        onClicked: {
                            if (model.hasModelChildren)
                                view.model.rootIndex = view.model.modelIndex(index)
                        }
                        Timer {
                            id: checkChildren
                            interval: 100
                            running: true
                            repeat: true
                            onTriggered: {
                                if (model.hasModelChildren) {
                                    button.enabled = true
                                    checkChildren.stop()
                                }
                            }
                        }
                    }
                    Text {
                        text: fileSize
                    }
                }
            }
        }
    }
}
