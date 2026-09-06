import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: window
    width: 440
    height: 380
    color: "#8f09ef"

    Knob {
        anchors.centerIn: parent
        size: 100
    }
}
