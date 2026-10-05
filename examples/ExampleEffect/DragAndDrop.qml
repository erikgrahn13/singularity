pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: root

    width: 300
    height: 300

    readonly property color accentColor: "#bf00ff"
    readonly property color panelColor: "#10091a"
    readonly property color panelBorderColor: "#33263d"
    readonly property color tileColor: "#1a1222"
    readonly property color tileHoverColor: "#25152f"
    readonly property color primaryTextColor: "#f4edf7"
    readonly property color secondaryTextColor: "#998da1"
    readonly property int panelGap: 8

    Rectangle {
        id: browserPanel

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: (root.width - root.panelGap) / 2
        color: root.panelColor
        border.color: root.panelBorderColor
        border.width: 1
        radius: 8

        Text {
            id: browserTitle

            anchors {
                left: parent.left
                top: parent.top
                margins: 12
            }
            color: root.primaryTextColor
            font {
                bold: true
                pixelSize: 12
                letterSpacing: 1
            }
            text: "TILE BROWSER"
        }

        Text {
            id: browserHint

            anchors {
                left: browserTitle.left
                top: browserTitle.bottom
                topMargin: 3
            }
            color: root.secondaryTextColor
            font.pixelSize: 10
            text: "Drag a tile into the slot"
        }

        ScrollView {
            anchors {
                left: parent.left
                right: parent.right
                top: browserHint.bottom
                bottom: parent.bottom
                margins: 8
                topMargin: 10
            }
            background: null
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            ListView {
                model: 20
                clip: true
                currentIndex: -1
                spacing: 5
                boundsBehavior: Flickable.StopAtBounds

                delegate: DragTile {
                    width: ListView.view.width
                    colorKey: root.tileColor
                    dragParent: dragLayer
                }
            }
        }
    }

    Rectangle {
        id: slotPanel

        anchors {
            left: browserPanel.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            leftMargin: root.panelGap
        }
        color: root.panelColor
        border.color: root.panelBorderColor
        border.width: 1
        radius: 8

        Text {
            id: slotTitle

            anchors {
                left: parent.left
                top: parent.top
                margins: 12
            }
            color: root.primaryTextColor
            font {
                bold: true
                pixelSize: 12
                letterSpacing: 1
            }
            text: "DROP SLOT"
        }

        Text {
            id: slotHint

            anchors {
                left: slotTitle.left
                top: slotTitle.bottom
                topMargin: 3
            }
            color: root.secondaryTextColor
            font.pixelSize: 10
            text: "One tile at a time"
        }

        DropTile {
            anchors {
                left: parent.left
                right: parent.right
                top: slotHint.bottom
                bottom: parent.bottom
                margins: 8
                topMargin: 10
            }
        }
    }

    Item {
        id: dragLayer

        anchors.fill: parent
        z: 100
    }

    component DragTile: Item {
        id: dragId

        required property color colorKey
        required property int modelData
        required property Item dragParent
        required property int index

        height: 34

        Rectangle {
            id: tile

            anchors.fill: parent
            color: mouseArea.containsMouse ? root.tileHoverColor : dragId.colorKey
            border.color: dragId.ListView.isCurrentItem
                          ? root.accentColor
                          : root.panelBorderColor
            border.width: dragId.ListView.isCurrentItem ? 2 : 1
            radius: 5

            Behavior on color {
                ColorAnimation { duration: 90 }
            }

            Rectangle {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                    margins: 6
                }
                width: 2
                color: root.accentColor
                opacity: dragId.ListView.isCurrentItem ? 1 : 0.35
                radius: 1
            }

            Text {
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: 15
                    rightMargin: 8
                }
                color: root.primaryTextColor
                elide: Text.ElideRight
                font {
                    pixelSize: 11
                    weight: Font.Medium
                }
                text: "Tile %1".arg(dragId.modelData + 1)
            }
        }

        MouseArea {
            id: mouseArea

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            drag.target: dragProxy

            onPressed: {
                dragId.ListView.view.currentIndex = dragId.index

                const position = tile.mapToItem(dragId.dragParent, 0, 0)
                dragProxy.x = position.x
                dragProxy.y = position.y
            }

            onReleased: {
                const target = dragProxy.Drag.target as DropTile
                if (target !== null)
                    target.droppedLabel = "Tile %1".arg(dragId.modelData + 1)
            }
        }

        Rectangle {
            id: dragProxy

            parent: dragId.dragParent
            visible: mouseArea.drag.active
            z: 1

            width: tile.width
            height: tile.height
            color: root.tileHoverColor
            border.color: root.accentColor
            border.width: 2
            radius: 5
            opacity: 0.96

            Drag.keys: [ "showcase-tile" ]
            Drag.active: mouseArea.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2

            Text {
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: 12
                }
                color: root.primaryTextColor
                elide: Text.ElideRight
                font {
                    pixelSize: 11
                    weight: Font.Medium
                }
                text: "Tile %1".arg(dragId.modelData + 1)
            }
        }
    }

    component DropTile: DropArea {
        id: dragTarget

        property string droppedLabel: ""

        keys: [ "showcase-tile" ]

        Rectangle {
            anchors.fill: parent
            color: dragTarget.containsDrag ? "#21102d" : "#0c0712"
            border.color: dragTarget.containsDrag
                          ? root.accentColor
                          : root.panelBorderColor
            border.width: dragTarget.containsDrag ? 2 : 1
            radius: 7

            Behavior on color {
                ColorAnimation { duration: 100 }
            }

            Behavior on border.color {
                ColorAnimation { duration: 100 }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 8
                color: "transparent"
                border.color: dragTarget.containsDrag ? "#8f3caf" : "#29202f"
                border.width: 1
                radius: 5
            }

            Column {
                anchors.centerIn: parent
                spacing: 5
                visible: dragTarget.droppedLabel.length === 0

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: dragTarget.containsDrag
                           ? root.accentColor
                           : root.secondaryTextColor
                    font.pixelSize: 24
                    text: "+"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: root.primaryTextColor
                    font {
                        bold: true
                        pixelSize: 11
                    }
                    text: dragTarget.containsDrag ? "Release to place" : "Drop tile here"
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 8
                visible: dragTarget.droppedLabel.length > 0

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 96
                    height: 36
                    color: root.tileColor
                    border.color: root.accentColor
                    border.width: 2
                    radius: 5

                    Text {
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            margins: 8
                        }
                        color: root.primaryTextColor
                        elide: Text.ElideRight
                        font {
                            pixelSize: 11
                            weight: Font.Medium
                        }
                        horizontalAlignment: Text.AlignHCenter
                        text: dragTarget.droppedLabel
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        cursorShape: Qt.PointingHandCursor

                        onClicked: dragTarget.droppedLabel = ""
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: root.secondaryTextColor
                    font.pixelSize: 9
                    text: "Right-click to clear"
                }
            }
        }
    }
}
