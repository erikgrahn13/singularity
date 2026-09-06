import QtQuick
import QtQuick.Shapes

Item {
  id: knob

  property real size: 40
  property int parameterId: -1
  property var parameterProvider: null
  property real value: 0.5
  property var theme: ({})
  property var draw: null
  property color backgroundColor: "transparent"
  readonly property var colors: ({
    bodyColor: theme.bodyColor ?? "#000000",
    trackColor: theme.trackColor ?? "#383850",
    valueColor: theme.valueColor ?? "#c880ff",
    valueShadow: theme.valueShadow ?? "#a030ff",
    dotColor: theme.dotColor ?? "#ffffff",
    bloom: theme.bloom ?? 1.2,
    trackWidth: theme.trackWidth ?? size * 0.13
  })

  implicitWidth: size
  implicitHeight: size

  function syncParameter() {
    if (!parameterProvider || parameterId < 0)
      return;
    const next = parameterProvider.getParameter(parameterId);
    if (Number.isFinite(next))
      value = Math.max(0, Math.min(1, next));
  }

  function editValue(next) {
    if (!Number.isFinite(next))
      return;
    value = Math.max(0, Math.min(1, next));
    if (parameterProvider && parameterId >= 0) {
      parameterProvider.setParameter(parameterId, value);
      syncParameter();
    }
  }

  onParameterIdChanged: syncParameter()
  onParameterProviderChanged: syncParameter()
  Component.onCompleted: syncParameter()

  // The shared parameter API has no change signal; follow host automation too.
  Timer {
    interval: 33
    repeat: true
    running: knob.visible && knob.parameterProvider !== null && knob.parameterId >= 0
    onTriggered: knob.syncParameter()
  }

  Rectangle {
    anchors.fill: parent
    color: knob.backgroundColor
  }

  // Keep the normal rendering in the scene graph. A changing Canvas shadow
  // otherwise rasterizes and uploads a new image on every drag update.
  Item {
    id: graphics
    anchors.fill: parent
    visible: !knob.draw
    readonly property real diameter: Math.min(width, height)
    readonly property real centerX: width / 2
    readonly property real centerY: height / 2
    readonly property real trackRadius: diameter * 0.36
    readonly property real trackWidth: knob.size > 0
      ? knob.colors.trackWidth * diameter / knob.size : 0
    readonly property real sweep: Math.max(0, Math.min(1, knob.value)) * 270
    readonly property real angle: (135 + sweep) * Math.PI / 180

    Rectangle {
      anchors.centerIn: parent
      width: graphics.diameter
      height: width
      radius: width / 2
      color: knob.colors.bodyColor
      antialiasing: true
    }

    Shape {
      anchors.fill: parent
      preferredRendererType: Shape.CurveRenderer

      ShapePath {
        fillColor: "transparent"
        strokeColor: knob.colors.trackColor
        strokeWidth: graphics.trackWidth
        capStyle: ShapePath.FlatCap
        PathAngleArc {
          centerX: graphics.centerX
          centerY: graphics.centerY
          radiusX: graphics.trackRadius
          radiusY: radiusX
          startAngle: 135
          sweepAngle: 270
        }
      }
    }

    // Translucent strokes approximate the glow without a raster shadow blur.
    Shape {
      anchors.fill: parent
      visible: graphics.sweep > 0 && knob.colors.bloom > 0
      opacity: 0.10 * Math.min(2, knob.colors.bloom)
      preferredRendererType: Shape.CurveRenderer
      ShapePath {
        fillColor: "transparent"
        strokeColor: knob.colors.valueShadow
        strokeWidth: graphics.trackWidth * 2
        capStyle: ShapePath.RoundCap
        PathAngleArc {
          centerX: graphics.centerX
          centerY: graphics.centerY
          radiusX: graphics.trackRadius
          radiusY: radiusX
          startAngle: 135
          sweepAngle: graphics.sweep
        }
      }
    }

    Shape {
      anchors.fill: parent
      visible: graphics.sweep > 0
      preferredRendererType: Shape.CurveRenderer
      ShapePath {
        fillColor: "transparent"
        strokeColor: knob.colors.valueColor
        strokeWidth: graphics.trackWidth
        capStyle: ShapePath.FlatCap
        PathAngleArc {
          centerX: graphics.centerX
          centerY: graphics.centerY
          radiusX: graphics.trackRadius
          radiusY: radiusX
          startAngle: 135
          sweepAngle: graphics.sweep
        }
      }
    }

    Rectangle {
      width: graphics.diameter * 0.18
      height: width
      radius: width / 2
      x: graphics.centerX + Math.cos(graphics.angle) * graphics.trackRadius - width / 2
      y: graphics.centerY + Math.sin(graphics.angle) * graphics.trackRadius - height / 2
      color: knob.colors.dotColor
      antialiasing: true
    }
  }

  // Preserve the optional custom painter without creating a Canvas by default.
  Loader {
    anchors.fill: parent
    active: knob.draw !== null
    sourceComponent: Component {
      Canvas {
        id: customCanvas
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections {
          target: knob
          function onValueChanged() { customCanvas.requestPaint(); }
          function onColorsChanged() { customCanvas.requestPaint(); }
          function onDrawChanged() { customCanvas.requestPaint(); }
        }
        onPaint: {
          const ctx = getContext("2d");
          ctx.reset();
          if (!knob.draw || knob.size <= 0 || width <= 0 || height <= 0)
            return;
          ctx.save();
          ctx.scale(width / knob.size, height / knob.size);
          knob.draw(ctx, { value: knob.value, size: knob.size, theme: knob.colors });
          ctx.restore();
        }
      }
    }
  }

  MouseArea {
    anchors.fill: parent
    property real lastY: 0
    preventStealing: true
    onPressed: mouse => {
      knob.syncParameter();
      lastY = mouse.y;
    }
    onPositionChanged: mouse => {
      if (!pressed || knob.size <= 0)
        return;
      const dy = mouse.y - lastY;
      lastY = mouse.y;
      knob.editValue(knob.value - dy * 0.3 / knob.size);
    }
    onWheel: wheel => {
      if (knob.size <= 0)
        return;
      knob.syncParameter();
      const delta = wheel.pixelDelta.y !== 0
        ? wheel.pixelDelta.y : wheel.angleDelta.y / 120;
      knob.editValue(knob.value + delta / knob.size);
      wheel.accepted = true;
    }
  }
}
