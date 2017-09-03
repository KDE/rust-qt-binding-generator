import QtQuick 2.6
import QtCharts 2.2

Item {
    ChartView {
        id: chart
        anchors.fill: parent
        Component.onCompleted: {
            legend.alignment = Qt.AlignBottom
        }

        antialiasing: true

        ValueAxis {
            id: axisX
            titleText: qsTr("time [s]")
            labelFormat: "%.1f"
        }

        ValueAxis {
            id: axisY
            titleText: qsTr("electric potential [V]")
            labelFormat: "%.1f"
        }

        LineSeries {
            id: sin
            name: "sin"
            axisX: axisX
            axisY: axisY
        }

        VXYModelMapper {
            model: demo.timeSeries
            xColumn: 0
            yColumn: 1
            series: sin
        }

        LineSeries {
            id: cos
            name: "cos"
            axisX: axisX
            axisY: axisY
        }

        VXYModelMapper {
            model: demo.timeSeries
            xColumn: 0
            yColumn: 2
            series: cos
        }
    }
}
