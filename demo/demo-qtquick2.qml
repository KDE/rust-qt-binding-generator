import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: application
    x: windowX
    y: windowY
    width: windowWidth
    height: windowHeight
    visible: true
    TabBar {
        id: bar
        width: parent.width
        TabButton { text: "style" }
        TabButton { text: "object" }
        TabButton { text: "list" }
        TabButton { text: "tree" }
    }
    StackLayout {
        width: parent.width
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        currentIndex: bar.currentIndex
        ColumnLayout {
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
        ListView {
            id: listView
            model: fibonacciList
            delegate: Row {
                Text { text: result }
            }
        }
        Text {
            id: treeView
            text: "No TreeView in QtQuick Controls 2"
        }
    }
}
