import { Component, getParameter, setParameter } from "singularity";

export function Knob({ x, y, size = 40, parameterId, theme = {} }) {
  let lastY = null;

  const t = {
    bodyColor: "#ffffff00",
    trackColor: "#3a3a5c",
    valueColor: "#8000ff",
    dotColor: "#ffffff",
    trackWidth: size * 0.13,
    ...theme,
  };

  return Component({
    x,
    y,
    width: size,
    height: size,

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

    draw: (ctx) => {
      const value = getParameter(parameterId);
      if (Number.isNaN(value)) return;

      ctx.fillStyle = "#000000";
      ctx.fillRect(0, 0, size, size);


      const radius = size / 2;
      const cx = radius;
      const cy = radius;

      const startAngle = 0.75 * Math.PI;
      const totalSweep = 1.5 * Math.PI;
      const valueAngle = startAngle + value * totalSweep;
      const trackRadius = radius * 0.72;
      const tw = t.trackWidth;

      ctx.save();

      ctx.beginPath();
      ctx.arc(cx, cy, radius, 0, Math.PI * 2);
      ctx.fill();

      ctx.beginPath();
      ctx.arc(cx, cy, trackRadius, startAngle, startAngle + totalSweep, false);
      ctx.strokeStyle = t.trackColor;
      ctx.lineWidth = tw;
      ctx.stroke();

      ctx.beginPath();
      ctx.arc(cx, cy, trackRadius, startAngle, valueAngle, false);
      ctx.strokeStyle = t.valueColor;
      ctx.lineWidth = tw;
      ctx.stroke();

      const dotX = cx + Math.cos(valueAngle) * trackRadius;
      const dotY = cy + Math.sin(valueAngle) * trackRadius;

      ctx.beginPath();
      ctx.arc(dotX, dotY, radius * 0.2, 0, Math.PI * 2);
      ctx.fillStyle = t.dotColor;
      ctx.fill();

      ctx.restore();
    },
  });
}
