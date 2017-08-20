import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3
import rust 1.0

ApplicationWindow {
    id: application
    x: windowX
    y: windowY
    width: windowWidth
    height: windowHeight
    visible: true
    ItemSelectionModel {
        id: selectionModel
        model: fsModel
    }
    TabView {
        anchors.fill: parent
        Tab {
            title: "style"
            ComboBox {
                currentIndex: qtquickIndex
                model: styles
                textRole: "display"
                onCurrentIndexChanged: {
                    if (currentText && currentText != "QtQuick") {
                        widgets.currentText = currentText;
                        application.close();
                    }
                }
            }
        }
        Tab {
            title: "object"
            RowLayout {
                TextField {
                    id: fibonacciInput
                    placeholderText: "Your number"
                    validator: IntValidator {bottom: 0; top: 100;}
                    Component.onCompleted: { text = fibonacci.input }
                    onTextChanged: { fibonacci.input = parseInt(text, 10) }
                }
                Text {
                    text: "The Fibonacci number: " + fibonacci.result
                }
            }
        }
        Tab {
            title: "list"
            TableView {
                id: listView
                model: fibonacciList
                TableViewColumn {
                    role: "display"
                    title: "Row"
                    width: 100
                }
            }
        }
        Tab {
            title: "tree"
            TreeView {
                id: treeView
                model: sortedFsModel
                selection: selectionModel
                TableViewColumn {
                    title: "Name"
                    role: "fileName"
                }
                TableViewColumn {
                    title: "Size"
                    role: "fileSize"
                }
                Component.onCompleted: {
                    var root = treeView.rootIndex;
                    var first = sortedFsModel.index(0, 0, root);
                    treeView.expand(first);
                }
            }
        }
    }
}
