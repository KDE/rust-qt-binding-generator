import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: application
    property string initialTab: "style"
    property int qtquickIndex: 0
    property var processes: ListModel {
        ListElement {
            name: "init"
        }
    }
    onInitialTabChanged: {
        for (var i = 0; i < tabView.count; ++i) {
            if (tabView.getTab(i).title === initialTab) {
                tabView.currentIndex = i
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
    statusBar: StatusBar {
        StyleSwitcher {
            anchors.right: parent.right
        }
    }
    TabView {
        id: tabView
        anchors.fill: parent
        Tab {
            title: "object"
            Fibonacci {}
        }
        Tab {
            title: "list"
            FibonacciList {}
        }
        Tab {
            title: "tree"
            FileTreeView {}
        }
        Tab {
            title: "processes"
            ProcessesTree {}
        }
        Tab {
            id: chartTab
            title: "chart"
            DataAndChart {}
        }
    }
}
