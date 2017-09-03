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
        anchors.fill: parent
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
