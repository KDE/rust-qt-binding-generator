import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

Column {
    Text {
        text: qsTr("Calculate the <i>nth</i> Fibonacci number")
    }
    TextField {
        id: fibonacciInput
        placeholderText: qsTr("Your number")
        validator: IntValidator {
            bottom: 0
            top: 100
        }
        text: demo.fibonacci.input
        onTextChanged: {
            var val = parseInt(text, 10)
            if (val !== demo.fibonacci.input) {
                demo.fibonacci.input = val
            }
        }
    }
    Text {
        text: qsTr("The Fibonacci number: ") + demo.fibonacci.result
    }
}
