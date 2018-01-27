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
import QtQml.Models 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: application
    property string initialTab: "style"
    property int qtquickIndex: 0
    property var processes: ListModel {
        ListElement {
            name: "init"
        }
    }
    onInitialTabChanged: {
        for (var i = 0; i < tabView.count; ++i) {
            if (tabView.getTab(i).title === initialTab) {
                tabView.currentIndex = i
            }
        }
    }
    width: 640
    height: 480
    visible: true
    ItemSelectionModel {
        id: selectionModel
        model: sortedFileSystem
    }
    ItemSelectionModel {
        id: processSelection
        model: processes
    }
    statusBar: StatusBar {
        StyleSwitcher {
            anchors.fill: parent
        }
    }
    TabView {
        id: tabView
        anchors.fill: parent
        Tab {
            title: "object"
            Fibonacci {}
        }
        Tab {
            title: "list"
            FibonacciList {}
        }
        Tab {
            title: "tree"
            FileTreeView {}
        }
        Tab {
            title: "processes"
            ProcessesTree {}
        }
        Tab {
            id: chartTab
            title: "chart"
            DataAndChart {}
        }
    }
}
