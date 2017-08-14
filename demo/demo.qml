import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import rust 1.0

ApplicationWindow {
    width: 300
    height: 480
    visible: true
    Directory {
        id: directory
        path: "/"
    }
    ItemSelectionModel {
        id: selectionModel
        model: fsModel
    }
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
/*
        TreeView {
            model: directory
            TableViewColumn {
                title: "Name"
                role: "fileName"
            }
            TableViewColumn {
                title: "Permissions"
                role: "filePermissions"
            }
        }
*/
        TreeView {
            model: fsModel
            selection: selectionModel
            //selectionMode: SelectionMode.SingleSelection
            TableViewColumn {
                title: "Name"
                role: "fileName"
                width: 200
            }
            TableViewColumn {
                title: "Size"
                role: "fileSize"
                width: 100
            }
/*
            itemDelegate: Item {
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    color: "blue"//styleData.textColor
                    elide: styleData.elideMode
                    text: styleData.index.column + " " + styleData.value
                }
            }
            onClicked: {
                selectionModel.setCurrentIndex(index, ItemSelectionModel.Select)
            }
            rowDelegate: Item {
                anchors.fill: parent
                Text {
                    text: styleData.row
                }
            }
*/
        }
/*
        TableView {
            model: fsModel
        }
*/
    }
}
