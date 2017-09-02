import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

RowLayout {
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
        onActivated: {
            if (index !== qtquickIndex) {
                widgets.currentIndex = index;
                application.close();
            }
        }
    }
}
