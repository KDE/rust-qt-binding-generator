import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: application
    x: windowX
    y: windowY
    width: windowWidth
    height: windowHeight
    visible: true
    ItemSelectionModel {
        id: selectionModel
        model: sortedFileSystem
    }
    TabView {
        id: tabView
        anchors.fill: parent
        onCountChanged: {
            for (var i = 0; i < tabView.count; ++i) {
                if (tabView.getTab(i).title == initialTab) {
                    tabView.currentIndex = i;
                }
            }
        }
        Tab {
            title: "style"
            Column {
                anchors.fill: parent
                anchors.margins: 5
                ComboBox {
                    currentIndex: qtquickIndex
                    model: styles
                    textRole: "display"
                    onCurrentIndexChanged: {
                        if (currentIndex != qtquickIndex) {
                            widgets.currentIndex = currentIndex;
                            application.close();
                        }
                    }
                }
                Image {
                    source: "logo.svg"
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
                model: sortedFileSystem
                sortIndicatorVisible: true
                alternatingRowColors: true
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
                    var first = sortedFileSystem.index(0, 0, root);
                    treeView.expand(first);
                }
            }
        }
        Tab {
            title: "processes"
            TreeView {
                id: processView
                model: processes
                sortIndicatorVisible: true
                alternatingRowColors: true
                TableViewColumn {
                    title: "pid"
                    role: "pid"
                }
                TableViewColumn {
                    title: "name"
                    role: "name"
                }
                TableViewColumn {
                    title: "cpu"
                    role: "cpu"
                }
                onSortIndicatorColumnChanged: {
                    switch (processView.sortIndicatorColumn) {
                    case 0: model.sortRole = Qt.DisplayRole; break;
                    }
                }
                onSortIndicatorOrderChanged: {
                    model.sort(Qt.DisplayRole, processView.sortIndicatorOrderChanged);
                }
                Component.onCompleted: {
                    var r = processView.rootIndex;
                    var a = processes.index(0, 0, r);
                    var b = processes.index(1, 0, r);
                    processView.expand(processView.rootIndex);
                    processView.expand(a);
                    processView.expand(b);
                    processes.rowsInserted.connect(function (index) {
                        processView.expand(processView.rootIndex);
                        processView.expand(index);
                    });
                }
            }
/*
    view->connect(&models->sortedProcesses, &QAbstractItemModel::rowsInserted,
        view, [view](const QModelIndex& index) {
        if (!index.isValid()) {
            view->expandAll();
        }
    });
            }
*/
        }
        Tab {
            id: chartTab
            title: "chart"
//            Row {
//                anchors.fill: parent
                Text {
                    text: "YAYAYA"
anchors.left: parent.left
anchors.right: i.left
                }
                Item {
id: i
anchors.right: parent.right
                    Text {
                        anchors.centerIn: parent
                        text: "QtChart is not available.";
                        visible: chartLoader.status !== Loader.Ready
                    }
                    Loader {
width: 100

                        id: chartLoader
        //                anchors.fill: parent
                        source: "chart.qml"
                    }
                }

//            }
        }
    }
}
