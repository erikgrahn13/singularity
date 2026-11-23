import QtQuick

Rectangle {
    id: pluginView
    
    readonly property int singularityPluginViewVersion: 1

    Component.onCompleted: Qt.callLater(() => {
        const overlay = pluginView.Overlay.overlay

        if (overlay && pluginView.parent
                && overlay.parent !== pluginView.parent) {
            overlay.parent = pluginView.parent
            overlay.anchors.fill = pluginView.parent
        }
    })
}
