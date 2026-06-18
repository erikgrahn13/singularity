import { Component, getParameter, setParameter } from "singularity";

export function Knob({ x, y, size = 40, parameterId, theme = {}, draw = null, backgroundColor }) {
  let lastY = null;

  const t = {
    bodyColor: "#000000",
    trackColor: "color(srgb-linear 0.04 0.04 0.08 1.0)",
    valueColor: "color(srgb-linear 4.0 1.0 8.0 1.0)",
    valueShadow: "color(srgb-linear 2.0 0.2 6.0 1.0)",
    dotColor: "#ffffff",
    dotShadow: "color(srgb-linear 6.0 4.0 10.0 1.0)",
    bloom: 1.2,
    trackWidth: size * 0.13,
    ...theme,
  };

  return Component({
    x,
    y,
    width: size,
    height: size,
    backgroundColor,

    onMouseDown: (e) => {
      lastY = e.y;
    },

    onMouseUp: (e) => {
      lastY = null;
    },

    onMouseDrag: (e) => {
      if (lastY === null) {
        lastY = e.y;
        return;
      }

      const dy = e.y - lastY;
      lastY = e.y;

      let value = getParameter(parameterId);
      if (Number.isNaN(value)) return;

      value = Math.max(0, Math.min(1, value - (dy * 0.3) / size));
      setParameter(parameterId, value);
    },

    onMouseWheel: (e) => {
      let value = getParameter(parameterId);
      if (Number.isNaN(value)) return;

      value = Math.max(0, Math.min(1, value + e.deltaY / size));
      setParameter(parameterId, value);
    },

    draw: (ctx) => {
      const value = getParameter(parameterId);
      if (Number.isNaN(value)) return;

      if (draw) {
        draw(ctx, { value, size, theme: t });
        return;
      }

      const radius = size / 2;
      const cx = radius;
      const cy = radius;

      const startAngle = 0.75 * Math.PI;
      const totalSweep = 1.5 * Math.PI;
      const valueAngle = startAngle + value * totalSweep;
      const trackRadius = radius * 0.72;
      const tw = t.trackWidth;

      ctx.save();

      ctx.bloom = t.bloom;

      // Body
      ctx.beginPath();
      ctx.arc(cx, cy, radius, 0, Math.PI * 2);
      ctx.fillStyle = t.bodyColor;
      ctx.fill();

      // Grey track
      ctx.beginPath();
      ctx.arc(cx, cy, trackRadius, startAngle, startAngle + totalSweep, false);
      ctx.strokeStyle = t.trackColor;
      ctx.lineWidth = tw;
      ctx.shadowBlur = 0;
      ctx.stroke();

      // Value arc — overbright bloom stroke, same recipe as the purple ring
      ctx.beginPath();
      ctx.arc(cx, cy, trackRadius, startAngle, valueAngle, false);
      ctx.strokeStyle = t.valueColor;
      ctx.shadowColor = t.valueShadow;
      ctx.shadowBlur  = tw * 2.5;
      ctx.lineWidth = tw;
      ctx.stroke();

      // Indicator dot — plain white, no glow
      const dotX = cx + Math.cos(valueAngle) * trackRadius;
      const dotY = cy + Math.sin(valueAngle) * trackRadius;

      ctx.beginPath();
      ctx.arc(dotX, dotY, radius * 0.18, 0, Math.PI * 2);
      ctx.fillStyle   = t.dotColor;
      ctx.shadowBlur  = 0;
      ctx.fill();

      ctx.restore();
    },
  });
}
