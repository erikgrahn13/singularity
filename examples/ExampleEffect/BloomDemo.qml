import QtQuick

Item {
    id: root

    property real bloomIntensity: 0.9
    property real hdrBoost: 3.5
    property real pulseWidth: 0.09
    property real threshold: 0.9
    property real time: 0.0

    readonly property size halfTextureSize: Qt.size(
        Math.max(1, Math.round(width * 0.5)),
        Math.max(1, Math.round(height * 0.5)))
    readonly property size quarterTextureSize: Qt.size(
        Math.max(1, Math.round(width * 0.25)),
        Math.max(1, Math.round(height * 0.25)))
    readonly property size eighthTextureSize: Qt.size(
        Math.max(1, Math.round(width * 0.125)),
        Math.max(1, Math.round(height * 0.125)))
    readonly property point halfHorizontalStep: Qt.point(1.0 / halfTextureSize.width, 0.0)
    readonly property point halfVerticalStep: Qt.point(0.0, 1.0 / halfTextureSize.height)
    readonly property point quarterHorizontalStep: Qt.point(1.0 / halfTextureSize.width, 0.0)
    readonly property point quarterVerticalStep: Qt.point(0.0, 1.0 / quarterTextureSize.height)
    readonly property point eighthHorizontalStep: Qt.point(1.0 / quarterTextureSize.width, 0.0)
    readonly property point eighthVerticalStep: Qt.point(0.0, 1.0 / eighthTextureSize.height)

    FrameAnimation {
        running: root.visible
        onTriggered: root.time += frameTime
    }

    ShaderEffect {
        id: sourcePass

        anchors.fill: parent
        blending: false

        property real iTime: root.time
        property size iResolution: Qt.size(width, height)
        property real hdrBoost: root.hdrBoost
        property real pulseWidth: root.pulseWidth

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_source.frag.qsb"
    }

    ShaderEffectSource {
        id: sourceTexture

        anchors.fill: parent
        sourceItem: sourcePass
        hideSource: true
        live: true
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: thresholdPass

        anchors.fill: parent
        blending: false

        property variant source: sourceTexture
        property real threshold: root.threshold

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_threshold.frag.qsb"
    }

    ShaderEffectSource {
        id: thresholdTexture

        anchors.fill: parent
        sourceItem: thresholdPass
        hideSource: true
        live: true
        textureSize: root.halfTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: horizontalBlurPass

        anchors.fill: parent
        blending: false

        property variant source: thresholdTexture
        property point texelStep: root.halfHorizontalStep

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_blur.frag.qsb"
    }

    ShaderEffectSource {
        id: horizontalBlurTexture

        anchors.fill: parent
        sourceItem: horizontalBlurPass
        hideSource: true
        live: true
        textureSize: root.halfTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: verticalBlurPass

        anchors.fill: parent
        blending: false

        property variant source: horizontalBlurTexture
        property point texelStep: root.halfVerticalStep

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_blur.frag.qsb"
    }

    ShaderEffectSource {
        id: halfBloomTexture

        anchors.fill: parent
        sourceItem: verticalBlurPass
        hideSource: true
        live: true
        textureSize: root.halfTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: quarterHorizontalBlurPass

        anchors.fill: parent
        blending: false

        property variant source: halfBloomTexture
        property point texelStep: root.quarterHorizontalStep

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_blur.frag.qsb"
    }

    ShaderEffectSource {
        id: quarterHorizontalBlurTexture

        anchors.fill: parent
        sourceItem: quarterHorizontalBlurPass
        hideSource: true
        live: true
        textureSize: root.quarterTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: quarterVerticalBlurPass

        anchors.fill: parent
        blending: false

        property variant source: quarterHorizontalBlurTexture
        property point texelStep: root.quarterVerticalStep

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_blur.frag.qsb"
    }

    ShaderEffectSource {
        id: quarterBloomTexture

        anchors.fill: parent
        sourceItem: quarterVerticalBlurPass
        hideSource: true
        live: true
        textureSize: root.quarterTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: eighthHorizontalBlurPass

        anchors.fill: parent
        blending: false

        property variant source: quarterBloomTexture
        property point texelStep: root.eighthHorizontalStep

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_blur.frag.qsb"
    }

    ShaderEffectSource {
        id: eighthHorizontalBlurTexture

        anchors.fill: parent
        sourceItem: eighthHorizontalBlurPass
        hideSource: true
        live: true
        textureSize: root.eighthTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        id: eighthVerticalBlurPass

        anchors.fill: parent
        blending: false

        property variant source: eighthHorizontalBlurTexture
        property point texelStep: root.eighthVerticalStep

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_blur.frag.qsb"
    }

    ShaderEffectSource {
        id: eighthBloomTexture

        anchors.fill: parent
        sourceItem: eighthVerticalBlurPass
        hideSource: true
        live: true
        textureSize: root.eighthTextureSize
        format: ShaderEffectSource.RGBA16F
    }

    ShaderEffect {
        anchors.fill: parent
        blending: false

        property variant source: sourceTexture
        property variant halfBloomSource: halfBloomTexture
        property variant quarterBloomSource: quarterBloomTexture
        property variant eighthBloomSource: eighthBloomTexture
        property real bloomIntensity: root.bloomIntensity

        fragmentShader: "qrc:/shaders/ExampleEffect/bloom_composite.frag.qsb"
    }
}
