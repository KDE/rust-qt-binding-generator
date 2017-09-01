import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: application
    property string initialTab: "style"
    property int qtquickIndex: 0
    property var processes: ListModel { ListElement{ name: "init"} }
    onInitialTabChanged: {
        for (var i = 0; i < tabView.count; ++i) {
            if (tabView.getTab(i).title === initialTab) {
                tabView.currentIndex = i;
            }
        }
    }
    width: 640
    height: 480
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
                        if (currentIndex !== qtquickIndex) {
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
                    Component.onCompleted: { text = demo.fibonacci.input }
                    onTextChanged: { demo.fibonacci.input = parseInt(text, 10) }
                }
                Text {
                    text: "The Fibonacci number: " + demo.fibonacci.result
                }
            }
        }
        Tab {
            title: "list"
            TableView {
                id: listView
                model: demo.fibonacciList
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
                    Binding {
                        target: demo.processes
                        property: "active"
                        value: visible
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
                                        return;
                                    }
                                    var val = loaderEditor.item.text;
                                    var row = styleData.row;
                                    if (styleData.column === 0) {
                                        demo.timeSeries.setInput(row, val);
                                    } else {
                                        demo.timeSeries.setResult(row, val);
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
                        title: "input"
                    }
                    TableViewColumn {
                        role: "result"
                        title: "result"
                    }
                    itemDelegate: {
                        return editableDelegate;
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Text {
                        anchors.centerIn: parent
                        text: "QtChart is not available.";
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
    }
}
