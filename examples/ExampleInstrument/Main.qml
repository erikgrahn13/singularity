import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Singularity

PluginView {
    id: window
    width: 440
    height: 380
    color: '#000000'

    // Dial {
    //     readonly property var parameter: controller.parameter(13)
    //     anchors.centerIn: parent
    //     inputMode: Dial.Vertical
    //     from: parameter.minimum
    //     to: parameter.maximum
    //     stepSize: parameter.stepSize
    //     enabled: !parameter.readOnly
    //     value: parameter.value
    //     onMoved: parameter.value = value
    // }

    Image {
        source: "resources/logo_transparent.png"
        sourceSize.width: 200
        sourceSize.height: 200
    }

    // Slider {
    //     readonly property var parameter: controller.parameter(13)
    //     anchors.horizontalCenter: parent.horizontalCenter
    //     anchors.bottom: parent.bottom
    //     anchors.bottomMargin: 24
    //     from: parameter.minimum
    //     to: parameter.maximum
    //     stepSize: parameter.stepSize
    //     enabled: !parameter.readOnly
    //     value: parameter.value
    //     onMoved: parameter.value = value
    // }

    readonly property QtObject gain: parameters.get(13)

    Dial{
        value: gain.value
        inputMode: Dial.Vertical
        onMoved: gain.value = value
    }
}
