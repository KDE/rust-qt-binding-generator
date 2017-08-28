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
    ItemSelectionModel {
        id: processSelection
        model: processes
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
                onSortIndicatorColumnChanged: sort()
                onSortIndicatorOrderChanged: sort()
                function sort() {
                    var role = getColumn(treeView.sortIndicatorColumn).role;
                    model.sortByRole(role, treeView.sortIndicatorOrder);
                }
            }
        }
        Tab {
            title: "processes"
            Item {
                anchors.fill: parent
                TextField {
                    id: processFilterInput
                    focus: true
                    width: parent.width
                    placeholderText: "Filter processes"
                    onTextChanged: {
                        processes.filterRegExp
                            = new RegExp(processFilterInput.text);
                    }
                }
                TreeView {
                    onClicked: {
                        processSelection.select(index, ItemSelectionModel.ToggleCurrent);
                    }
                    Component.onCompleted: {
                        processes.active = Qt.binding(function() { return visible; });
                    }
                    width: parent.width
                    anchors.top: processFilterInput.bottom
                    anchors.bottom: parent.bottom
                    id: processView
                    model: processes
                    selection: processSelection
                    selectionMode: SelectionMode.ExtendedSelection
//                    selectionMode: SelectionMode.SingleSelection
                    sortIndicatorVisible: true
                    alternatingRowColors: true
                    TableViewColumn {
                        title: "name"
                        role: "name"
                    }
                    TableViewColumn {
                        title: "cpu"
                        role: "cpuUsage"
                    }
                    TableViewColumn {
                        title: "memory"
                        role: "memory"
                    }
                    onSortIndicatorColumnChanged: sort()
                    onSortIndicatorOrderChanged: sort()
                    function sort() {
                        var role = getColumn(processView.sortIndicatorColumn).role;
                        model.sortByRole(role, processView.sortIndicatorOrder);
                    }
                    Timer {
                        interval: 100; running: true; repeat: true
                        onTriggered: {
                            var root = processView.rootIndex;
                            var systemd = processes.index(1, 0, root);
                            if (processes.data(systemd) === "systemd") {
                                processView.expand(systemd);
                                running = false;
                            }
                        }
                    }
                }
            }
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
