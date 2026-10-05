import QtQuick

Rectangle{
    id: pianoRoll
    width: 100
    height: 160
    color: "red"

    readonly property var blackKeyPositions: [1, 2, 4, 5, 6]


    component WhiteKey : Rectangle {
        width: 40
        height: 160
        border.color: "#222"
        color: "white"

        // color: mouseArea.containsMouse ? "darkgray" : "white"


        MouseArea{
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true

            onPressed: {
                parent.color = "gray"
            }

            onReleased: {
                parent.color = "darkgray"
            }

            onEntered: {
                parent.color = "darkgray"
            }

            onExited: {
                parent.color = "white"
            }
        }
    }

    component BlackKey : Rectangle {
        width: 24
        height: 100
        color: "#111"
        z: 1

        // color: mouseArea.containsMouse ? "gray" : "#111"

        MouseArea{
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true

            onPressed: {
                parent.color = "gray"
            }

            onReleased: {
                parent.color = "#111"
            }

            onEntered: {
                parent.color = "darkgray"
            }

            onExited: {
                parent.color = "#111"
            }
        }

    }

    Row{
        spacing: 0

        Repeater {
            model: 7

            WhiteKey{
            }
        }



    }

    Repeater {
    model: pianoRoll.blackKeyPositions

    BlackKey {
        x: modelData * 40 - width / 2
    }
}
}
