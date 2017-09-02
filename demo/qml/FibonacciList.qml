import QtQuick 2.6
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

TableView {
    id: listView
    model: demo.fibonacciList
    TableViewColumn {
        delegate: Text {
            text: styleData.row + 1
        }
        title: "Row"
        width: 100
        Component.onCompleted: resizeToContents()
    }
    TableViewColumn {
        role: "display"
        title: "Fibonacci Number"
        width: 100
        Component.onCompleted: resizeToContents()
    }
}
