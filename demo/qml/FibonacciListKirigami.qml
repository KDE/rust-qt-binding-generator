import QtQuick 2.6
import QtQuick.Controls 2.0 as QQC2
import org.kde.kirigami 2.0 as Kirigami

Kirigami.ScrollablePage {
    ListView {
        model: demo.fibonacciList
        header: Kirigami.Heading {
            text: qsTr("Row") + "\t" + qsTr("Fibonacci number")
        }
        delegate: Kirigami.BasicListItem {
            label: row + "\t" + fibonacciNumber
        }
    }
}
