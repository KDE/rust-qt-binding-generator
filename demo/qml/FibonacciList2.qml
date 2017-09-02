import QtQuick 2.6
import QtQuick.Controls 2.2

ListView {
    model: demo.fibonacciList
    header: Row {
        Text {
            width: 100
            text: qsTr("Row")
        }
        Text {
            text: qsTr("Fibonacci number")
        }
    }
    delegate: Row {
        Text {
            width: 100
            text: row
        }
        Text {
            text: fibonacciNumber
        }
    }
}
