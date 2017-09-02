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
    footer: StyleSwitcher2 {
    }
    StackLayout {
        anchors.fill: parent
        currentIndex: bar.currentIndex
        Fibonacci2 {}
        FibonacciList2 {}
        Text {
            id: treeView
            text: "No TreeView in QtQuick Controls 2"
        }
    }
}
