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

import QtQuick 2.1
import QtQuick.Controls 2.0 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Layouts 1.3

Kirigami.ApplicationWindow {
    id: application
    property string initialTab: "style"
    property int qtquickIndex: 0
    visible: true
    width: 800
    height: 640
    header: QQC2.TabBar {
        id: bar
        width: parent.width
        QQC2.TabButton {
            text: "object"
        }
        QQC2.TabButton {
            text: "list"
        }
        QQC2.TabButton {
            text: "tree"
        }
    }

    footer: Rectangle {
        height: statusBar.height
        width: parent.width
        StyleSwitcher2 {
            id: statusBar
            width: parent.width
        }
    }
    StackLayout {
        anchors.fill: parent
        currentIndex: bar.currentIndex
        Fibonacci2 {}
        FibonacciListKirigami {}
        FileTreeViewKirigami {}
    }
}
