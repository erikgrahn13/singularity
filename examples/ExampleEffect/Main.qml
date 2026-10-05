pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore
import Singularity
import "./example.mjs" as Hej

PluginView {
    id: root

    width: 900
    height: 700
    color: "#0b0711"

    readonly property color accentColor: "#bf00ff"
    readonly property color panelColor: "#10091a"
    readonly property color panelBorderColor: "#33263d"
    readonly property color primaryTextColor: "#f4edf7"
    readonly property color secondaryTextColor: "#998da1"
    readonly property QtObject gain: parameters.get(13)
    property bool gainControlsEnabled: true
    property var activeFileDialog: null
    property var activeFolderDialog: null

    function openFileDialog() {
        if (activeFileDialog !== null)
            return

        activeFileDialog = fileDialogComponent.createObject(root)
        if (activeFileDialog !== null)
            activeFileDialog.open()
    }

    function releaseFileDialog(dialog) {
        if (activeFileDialog === dialog)
            activeFileDialog = null

        dialog.destroy()
    }

    function openFolderDialog() {
        if (activeFolderDialog !== null)
            return

        activeFolderDialog = folderDialogComponent.createObject(root)
        if (activeFolderDialog !== null)
            activeFolderDialog.open()
    }

    function releaseFolderDialog(dialog) {
        if (activeFolderDialog === dialog)
            activeFolderDialog = null

        dialog.destroy()
    }

    Material.theme: Material.Dark
    Material.accent: accentColor

    ColumnLayout {
        anchors {
            fill: parent
            margins: 16
        }
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: root.panelColor
            border.color: root.panelBorderColor
            border.width: 1
            radius: 8

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: "#1a1025" }
                GradientStop { position: 1; color: "#10091a" }
            }

            Image {
                id: logo

                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                    leftMargin: 12
                }
                width: 38
                height: 38
                fillMode: Image.PreserveAspectFit
                source: "resources/logo_transparent.png"
                sourceSize: Qt.size(76, 76)
            }

            Column {
                anchors {
                    left: logo.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: 10
                }
                spacing: 1

                Text {
                    color: root.primaryTextColor
                    font {
                        bold: true
                        pixelSize: 15
                        letterSpacing: 1
                    }
                    text: "SINGULARITY"
                }

                Text {
                    color: root.secondaryTextColor
                    font.pixelSize: 10
                    text: "Qt Quick widget showcase"
                }
            }

            Rectangle {
                id: exampleBadgeBackground

                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    rightMargin: 12
                }
                width: exampleBadge.implicitWidth + 18
                height: 24
                color: "#21102d"
                border.color: "#573069"
                border.width: 1
                radius: 12

                Text {
                    id: exampleBadge

                    anchors.centerIn: parent
                    color: root.accentColor
                    font {
                        bold: true
                        pixelSize: 9
                        letterSpacing: 0.8
                    }
                    text: "EXAMPLE EFFECT"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 320
                color: root.panelColor
                border.color: root.panelBorderColor
                border.width: 1
                radius: 8

                Text {
                    id: controlsTitle

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
                    text: "PARAMETER CONTROLS"
                }

                Text {
                    anchors {
                        left: controlsTitle.left
                        top: controlsTitle.bottom
                        topMargin: 3
                    }
                    color: root.secondaryTextColor
                    font.pixelSize: 10
                    text: "Interactive and host-bound widgets"
                }

                GridLayout {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: controlsTitle.bottom
                        bottom: parent.bottom
                        margins: 10
                        topMargin: 24
                    }
                    columns: 2
                    columnSpacing: 8
                    rowSpacing: 8
                    uniformCellWidths: true
                    uniformCellHeights: true

                    WidgetCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "GAIN DIAL"
                        valueText: "%1%".arg(Math.round(root.gain.value * 100))

                        Dial {
                            anchors.centerIn: parent
                            width: 58
                            height: width
                            enabled: root.gainControlsEnabled
                            value: root.gain.value
                            inputMode: Dial.Vertical
                            wheelEnabled: true
                            stepSize: 0.02
                            Material.accent: root.accentColor

                            onValueChanged: root.gain.value = value
                        }
                    }

                    WidgetCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "GAIN SLIDER"
                        valueText: root.gain.value.toFixed(2)

                        Slider {
                            anchors.centerIn: parent
                            width: parent.width - 12
                            enabled: root.gainControlsEnabled
                            value: root.gain.value
                            wheelEnabled: true
                            stepSize: 0.02
                            Material.accent: root.accentColor

                            onValueChanged: root.gain.value = value
                        }
                    }

                    WidgetCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "GAIN ENABLE"
                        valueText: showcaseSwitch.checked ? "ON" : "OFF"

                        Switch {
                            id: showcaseSwitch

                            anchors.centerIn: parent
                            checked: root.gainControlsEnabled
                            text: checked ? "Enabled" : "Disabled"
                            font.pixelSize: 12
                            Material.accent: root.accentColor

                            onToggled: root.gainControlsEnabled = checked
                        }
                    }

                    WidgetCard {
                        id: counterCard

                        property int counter: Hej.exampleSquareFunction(2)

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "BUTTON"
                        valueText: counter

                        Button {
                            id: incrementButton

                            anchors.centerIn: parent
                            hoverEnabled: true
                            text: "Increment"
                            font.pixelSize: 12
                            Material.background: down
                                                 ? "#8e00bd"
                                                 : hovered ? "#d02cff" : root.accentColor
                            Material.foreground: "white"

                            onClicked: counterCard.counter++
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    text: "Open file dialog"
                    font.pixelSize: 11
                    Material.foreground: root.primaryTextColor

                    onClicked: root.openFileDialog()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    text: "Open folder dialog"
                    font.pixelSize: 11
                    Material.foreground: root.primaryTextColor

                    onClicked: root.openFolderDialog()
                }

                DragAndDrop {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 240

            WidgetCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: " SHADER"
                valueText: "ANIMATED"

                ShaderEffect {
                    id: shader

                    anchors.fill: parent
                    blending: false

                    property real iTime: 0.0
                    property size iResolution: Qt.size(width, height)

                    fragmentShader: "qrc:/shaders/ExampleEffect/test.frag.qsb"

                    FrameAnimation {
                        running: true
                        onTriggered: shader.iTime += frameTime
                    }
                }
            }

            WidgetCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "BLOOM"
                valueText: "MULTI-PASS"

                BloomDemo {
                    anchors.fill: parent
                }
            }
        }
    }

    Component {
        id: fileDialogComponent

        FileDialog {
            id: dialog

            title: "Choose a file"
            popupType: Popup.Window
            currentFolder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            nameFilters: [ "Audio and image files (*.wav *.aiff *.flac *.png *.jpg)",
                           "All files (*)" ]

            onAccepted: {
                plugin.openFile("sample", selectedFile)
                root.releaseFileDialog(dialog)
            }
            onRejected: root.releaseFileDialog(dialog)
        }
    }

    Component {
        id: folderDialogComponent

        FolderDialog {
            id: dialog

            title: "Choose a folder"
            popupType: Popup.Window
            currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

            onAccepted: root.releaseFolderDialog(dialog)
            onRejected: root.releaseFolderDialog(dialog)
        }
    }

    component WidgetCard: Rectangle {
        id: card

        required property string title
        property string valueText
        default property alias content: contentHost.data

        implicitWidth: 140
        implicitHeight: 110
        color: "#0c0712"
        border.color: root.panelBorderColor
        border.width: 1
        radius: 6

        Text {
            anchors {
                left: parent.left
                top: parent.top
                margins: 9
            }
            color: root.secondaryTextColor
            font {
                bold: true
                pixelSize: 11
                letterSpacing: 0.8
            }
            text: card.title
        }

        Text {
            anchors {
                right: parent.right
                top: parent.top
                margins: 9
            }
            color: root.accentColor
            font {
                bold: true
                pixelSize: 11
            }
            text: card.valueText
        }

        Item {
            id: contentHost

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: parent.bottom
                leftMargin: 8
                rightMargin: 8
                topMargin: 30
                bottomMargin: 6
            }
        }
    }
}
