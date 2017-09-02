import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Item {
    anchors.fill: parent

    ListView {
        id: view
        anchors.fill: parent
        model: DelegateModel {
            id: dirModel
            model: sortedFileSystem
            delegate: Item {
                width: parent.width
                height: row.height
                Row {
                    id: row
                    Text {
                        width: 200
                        text: fileName
                    }
                    Text {
                        text: fileSize
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (model.hasModelChildren)
                            view.model.rootIndex = view.model.modelIndex(index)
                    }
                }
            }
        }
        header: Row {
            Text {
                width: 200
                text: qsTr("Name")
            }
            Text {
                text: qsTr("Size")
            }
        }
    }
}
