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

        DateTimeAxis {
            id: axisX
            titleText: "Date"
            format: "MMM yyyy"
            tickCount: 10
        }

        ValueAxis {
            id: axisY
            titleText: "Sunspots count"
        }

        LineSeries {
            id: cpu
            name: "CPU"
            axisX: axisX
            axisY: axisY
        }

        VXYModelMapper {
            model: demo.timeSeries
            xColumn: 0
            yColumn: 1
            series: cpu
        }
    }
}
