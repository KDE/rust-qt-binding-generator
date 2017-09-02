import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: application
    property string initialTab: "style"
    property int qtquickIndex: 0
    visible: true
    height: 500
    header: TabBar {
        id: bar
        width: parent.width
        TabButton {
            text: "object"
        }
        TabButton {
            text: "list"
        }
        TabButton {
            text: "tree"
        }
    }
    footer: Rectangle {
        height: statusBar.height
        width: parent.width
        StyleSwitcher2 {
            id: statusBar
            width: parent.width
        }
    }
    StackLayout {
        anchors.fill: parent
        currentIndex: bar.currentIndex
        Fibonacci2 {}
        FibonacciList2 {}
        FileTreeView2 {}
    }
}
