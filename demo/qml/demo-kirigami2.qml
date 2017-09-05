import QtQuick 2.1
import QtQuick.Controls 2.0 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Layouts 1.3

Kirigami.ApplicationWindow {
    id: application
    property string initialTab: "style"
    property int qtquickIndex: 0
    visible: true
    width: 800
    height: 640
    header: QQC2.TabBar {
        id: bar
        width: parent.width
        QQC2.TabButton {
            text: "object"
        }
        QQC2.TabButton {
            text: "list"
        }
        QQC2.TabButton {
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
        FibonacciListKirigami {}
        FileTreeViewKirigami {}
    }
}
