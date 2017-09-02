import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

RowLayout {
    id: content
    Layout.fillWidth: true
    Image {
        sourceSize.height: 2* box.height
        fillMode: Image.PreserveAspectFit
        source: "../logo.svg"
    }
    ComboBox {
        id: box
        currentIndex: qtquickIndex
        model: styles
        textRole: "display"
        onCurrentIndexChanged: {
            if (currentIndex !== qtquickIndex) {
                widgets.currentIndex = currentIndex;
                application.close();
            }
        }
    }
}
