import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

RowLayout {
    anchors.fill: parent
    Component {
        id: editableDelegate
        Item {
            Text {
                text: styleData.value
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
                            demo.timeSeries.setInput(row, val)
                        } else {
                            demo.timeSeries.setResult(row, val)
                        }
                    }
                }
                sourceComponent: styleData.selected ? editor : null
            }
            Component {
                id: editor
                TextInput {
                    id: textInput
                    text: styleData.value
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: textInput.forceActiveFocus()
                    }
                }
            }
        }
    }
    TableView {
        model: demo.timeSeries
        Layout.fillHeight: true
        
        TableViewColumn {
            role: "input"
            title: qsTr("input")
        }
        TableViewColumn {
            role: "result"
            title: qsTr("result")
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
