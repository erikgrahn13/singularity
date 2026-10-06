pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes

Item {
    id: root

    required property var source

    property color waveformColor: "#70f6ff"
    property color fillColor: "#2e70f6ff"
    property color backgroundColor: "#12091f"
    property color gridColor: "#14ffffff"
    property int windowMs: 1500
    property int historyFrames: 0
    property real gain: 2
    property bool autoGain: false
    property real maxGain: 12

    property var history: []
    property int historyStart: 0
    property int historyOffset: 0
    property int lastRevision: 0
    property int sourceSampleRate: 48000
    property real displayGain: Math.max(0, gain)
    property bool gainInitialized: false
    property double previousTime: Date.now()
    property var upper: []
    property var lower: []
    property var envelope: []

    function trimHistory() {
        const visibleFrames = Math.max(
            2,
            Math.round(sourceSampleRate * windowMs / 1000))
        const capacity = Math.max(
            2,
            historyFrames > 0 ? historyFrames : visibleFrames * 2)
        const activeFrames = history.length - historyStart
        if (activeFrames > capacity)
            historyStart += activeFrames - capacity

        if (historyStart > capacity) {
            history.splice(0, historyStart)
            historyOffset += historyStart
            historyStart = 0
        }
    }

    function appendAudioData() {
        if (!source || source.revision === lastRevision)
            return false

        lastRevision = source.revision
        sourceSampleRate = source.sampleRate > 0
                         ? source.sampleRate
                         : sourceSampleRate

        const interleaved = source.samples ?? []
        const channels = Math.max(1, source.numChannels ?? 1)
        const frames = Math.floor(interleaved.length / channels)

        for (let frame = 0; frame < frames; ++frame) {
            let sample = 0
            for (let channel = 0; channel < channels; ++channel) {
                const candidate = interleaved[frame * channels + channel] ?? 0
                if (Math.abs(candidate) > Math.abs(sample))
                    sample = candidate
            }
            history.push(Math.max(-1, Math.min(1, sample)))
        }

        trimHistory()
        return true
    }

    function appendSilence(elapsedSeconds) {
        const visibleFrames = Math.max(
            2,
            Math.round(sourceSampleRate * windowMs / 1000))
        const silentFrames = Math.min(
            visibleFrames,
            Math.max(0, Math.round(sourceSampleRate * elapsedSeconds)))
        for (let frame = 0; frame < silentFrames; ++frame)
            history.push(0)

        trimHistory()
    }

    function updateDisplayGain(start, end) {
        if (!autoGain) {
            displayGain = Math.max(0, gain)
            return
        }

        let peak = 0
        for (let index = start; index < end; ++index)
            peak = Math.max(peak, Math.abs(history[index]))

        const target = peak > 0.0001
                     ? Math.max(1, Math.min(maxGain, 0.82 / peak))
                     : 1

        if (!gainInitialized) {
            displayGain = target
            gainInitialized = true
            previousTime = Date.now()
            return
        }

        const now = Date.now()
        const elapsed = Math.min(100, Math.max(0, now - previousTime))
        previousTime = now
        const responseMs = target < displayGain ? 35 : 180
        const amount = 1 - Math.exp(-elapsed / responseMs)
        displayGain += (target - displayGain) * amount
    }

    function updateGeometry() {
        const availableFrames = history.length - historyStart
        if (availableFrames < 2) {
            upper = []
            lower = []
            envelope = []
            return
        }

        const requestedFrames = Math.max(
            2,
            Math.round(sourceSampleRate * windowMs / 1000))
        const latestFrame = historyOffset + history.length
        const firstAvailableFrame = historyOffset + historyStart
        const displayStartFrame = latestFrame - requestedFrames
        const visibleStartFrame = Math.max(
            firstAvailableFrame,
            displayStartFrame)
        const start = visibleStartFrame - historyOffset
        const end = history.length
        updateDisplayGain(start, end)

        const columns = Math.max(1, Math.floor(width))
        const framesPerColumn = Math.max(
            1,
            Math.ceil(requestedFrames / columns))
        const firstBucket = Math.floor(
            visibleStartFrame / framesPerColumn) * framesPerColumn
        const centerY = height * 0.5
        const amplitude = Math.max(1, height * 0.44)
        const nextUpper = []
        const nextLower = []

        for (let bucket = firstBucket;
             bucket < latestFrame;
             bucket += framesPerColumn) {
            const first = Math.max(firstAvailableFrame, bucket) - historyOffset
            const last = Math.min(
                latestFrame,
                bucket + framesPerColumn) - historyOffset
            let minimum = 1
            let maximum = -1

            for (let index = first; index < last; ++index) {
                minimum = Math.min(minimum, history[index])
                maximum = Math.max(maximum, history[index])
            }

            const x = (bucket - displayStartFrame) * width / requestedFrames
            nextUpper.push(Qt.point(
                x,
                centerY - Math.max(
                    -1,
                    Math.min(1, maximum * displayGain)) * amplitude))
            nextLower.push(Qt.point(
                x,
                centerY - Math.max(
                    -1,
                    Math.min(1, minimum * displayGain)) * amplitude))
        }

        if (nextUpper.length === 0) {
            upper = []
            lower = []
            envelope = []
            return
        }

        const nextEnvelope = []
        for (let point = 0; point < nextUpper.length; ++point)
            nextEnvelope.push(nextUpper[point])
        for (let point = nextLower.length - 1; point >= 0; --point)
            nextEnvelope.push(nextLower[point])
        nextEnvelope.push(nextUpper[0])

        upper = nextUpper
        lower = nextLower
        envelope = nextEnvelope
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    Repeater {
        model: 3

        Rectangle {
            required property int index

            x: Math.round(root.width * (index + 1) / 4)
            width: 1
            height: root.height
            color: root.gridColor
        }
    }

    Repeater {
        model: 3

        Rectangle {
            required property int index

            y: Math.round(root.height * (index + 1) / 4)
            width: root.width
            height: 1
            color: root.gridColor
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: root.fillColor
            strokeColor: "transparent"

            PathPolyline {
                path: root.envelope
            }
        }

        ShapePath {
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            joinStyle: ShapePath.RoundJoin
            strokeColor: root.waveformColor
            strokeWidth: 1.5

            PathPolyline {
                path: root.upper
            }
        }

        ShapePath {
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            joinStyle: ShapePath.RoundJoin
            strokeColor: root.waveformColor
            strokeWidth: 1.5

            PathPolyline {
                path: root.lower
            }
        }
    }

    FrameAnimation {
        running: root.visible && root.source !== null
        onTriggered: {
            root.source.update()
            if (!root.appendAudioData())
                root.appendSilence(frameTime)
            root.updateGeometry()
        }
    }
}
