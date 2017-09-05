/*
 *   Copyright 2017  Jos van den Oever <jos@vandenoever.info>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
