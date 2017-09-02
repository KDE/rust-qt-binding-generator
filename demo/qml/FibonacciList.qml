import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

TableView {
    model: demo.fibonacciList
    TableViewColumn {
        role: "row"
        title: qsTr("Row")
        width: 100
        Component.onCompleted: resizeToContents()
    }
    TableViewColumn {
        role: "fibonacciNumber"
        title: qsTr("Fibonacci number")
        width: 100
        Component.onCompleted: resizeToContents()
    }
}
