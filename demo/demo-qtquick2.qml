import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import rust 1.0

ApplicationWindow {
    width: 500
    height: 480
    visible: true
    ItemSelectionModel {
        id: selectionModel
        model: fsModel
    }
    FibonacciList {
        id: fibonacciList
    }
    TabView {
        anchors.fill: parent
        Tab {
            title: "object"
            RowLayout {
                TextField {
                    id: fibonacciInput
                    placeholderText: "Your number"
                    validator: IntValidator {bottom: 0; top: 100;}
                }
                Text {
                    text: "The Fibonacci number: " + fibonacci.result
                }
                Fibonacci {
                    id: fibonacci
                    input: parseInt(fibonacciInput.text, 10)
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
