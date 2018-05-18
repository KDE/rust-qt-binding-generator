import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import RustCode 1.0;

ApplicationWindow {
    visible: true
    width: 450
    height: 580
    header: ToolBar {
        Label {
            anchors.fill: parent
            text: qsTr("todos")
            font.pixelSize: 30
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Component.onCompleted: {
        input.forceActiveFocus()
    }

    Todos {
        id: todoModel

        Component.onCompleted: {
            add("write bindings.json")
            add("run rust_qt_binding_generator")
            add("check bindings.h")
            add("check bindings.cpp")
            add("check interface.rs")
            add("write implementation.rs")
            add("write main.qml")
        }
    }

    Component {
        id: todoDelegate
        RowLayout {
            // the active tab determines if this item should be shown
            // 0: all, 1: active, 2: completed
            property bool show: filter.currentIndex === 0
                || (filter.currentIndex === 1 && !completed)
                || (filter.currentIndex === 2 && completed)
            visible: show
            width: parent.width
            height: show ? implicitHeight : 0
            CheckBox {
                checked: completed
                onToggled: todoModel.setCompleted(index, checked)
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    id: label
                    visible: !editInput.visible
                    text: description
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    font.strikeout: completed
                    font.pixelSize: 20
                }
                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onDoubleClicked: {
                        editInput.text = label.text
                        editInput.visible = true
                        editInput.forceActiveFocus()
                    }
                }
                Button {
                    text: 'X'
                    visible: (mouse.containsMouse && !editInput.visible)
                             || closeMouse.containsMouse
                    anchors.right: parent.right
                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: todoModel.remove(index)
                    }
                }
                TextField {
                    id: editInput
                    visible: false
                    anchors.fill: parent
                    text: description
                    font.pixelSize: label.font.pixelSize
                    onAccepted: {
                        todoModel.setDescription(index, text)
                        visible = false
                    }
                    onActiveFocusChanged: {
                        // hide when focus is lost
                        if (!activeFocus) {
                            visible = false
                        }
                    }
                    Keys.onPressed: {
                        // on escape, set value, hide (and lose focus)
                        if (event.key === Qt.Key_Escape) {
                            todoModel.setDescription(index, text)
                            visible = false
                            event.accepted = true
                        }
                    }
                }
            }
        }
    }

    Pane {
        anchors.fill: parent
        leftPadding: 0
        Page {
            anchors.fill: parent
            header: RowLayout {
                CheckBox {
                    tristate: true
                    // if there are no todos, do not show this checkbox
                    // but let it take up the same space
                    enabled: todoModel.count > 0
                    opacity: todoModel.count === 0 ? 0 : 1
                    checkState: {
                        if (todoModel.activeCount === 0) {
                            return Qt.Checked
                        } else if (todoModel.activeCount >= todoModel.count) {
                            return Qt.Unchecked
                        }
                        return Qt.PartiallyChecked
                    }
                    onCheckStateChanged: {
                        // if the change is triggered by a user action on this
                        // checkbox, check or uncheck all todos
                        // otherwise, do nothing
                        // (onToggle does not emit for tristate buttons)
                        if (activeFocus) {
                            var checked = checkState !== Qt.Unchecked
                            todoModel.setAll(checked)
                        }
                    }
                }
                TextField {
                    id: input
                    Layout.fillWidth: true
                    placeholderText: qsTr("What needs to be done?")
                    onAccepted: {
                        const todo = text.trim()
                        if (todo) {
                            todoModel.add(todo)
                        }
                        input.clear()
                    }
                }
            }
            Flickable {
                anchors.fill: parent
                ListView {
                    anchors.fill: parent
                    model: todoModel
                    delegate: todoDelegate
                }
            }
        }
    }

    footer: Pane {
        padding: 0
        ColumnLayout {
            width: parent.width
            TabBar {
                id: filter
                Layout.fillWidth: true
                visible: todoModel.count > 0
                TabButton {
                    text: qsTr("All")
                    checked: true
                }
                TabButton {
                    text: qsTr("Active")
                }
                TabButton {
                    text: qsTr("Completed")
                }
            }
            RowLayout {
                visible: todoModel.count > 0
                width: parent.width
                Label {
                    Layout.fillWidth: true
                    text: (todoModel.activeCount === 1)
                        ? qsTr("1 item left")
                        : todoModel.activeCount + qsTr(" items left")
                }
                Button {
                    enabled: todoModel.count > todoModel.activeCount
                    opacity: enabled
                    text: qsTr("Clear completed")
                    onClicked: todoModel.clearCompleted()
                }
            }
        }
    }
}
