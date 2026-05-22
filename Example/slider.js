import { Component, getParameter, setParameter } from "singularity";

// Slider component — horizontal or vertical, bound to a parameter
// Props:
//   x, y, width, height   — position and size
//   parameterId            — parameter to bind to (normalized 0..1)
//   vertical               — true for vertical, default false (horizontal)
//   theme                  — visual overrides (see defaults below)
export function Slider({ x = 0, y = 0, width = 160, height = 20,
                         parameterId, vertical = false, theme = {} }) {
  const trackThickness = vertical ? width * 0.25 : height * 0.25;
  const thumbRadius    = vertical ? width * 0.45 : height * 0.45;

  const t = {
    trackColor:      "#1e1e2e",
    fillColor:       "#6600d4ff",
    fillActiveColor: "#aa00d4ff",
    borderColor:     "#3a3a5c",
    thumbColor:      "#00d4ff",
    thumbHoverColor: "#44eeff",
    thumbPressColor: "#0099cc",
    thumbBorder:     "#44ffffff",
    cornerRadius:    trackThickness / 2,
    trackThickness,
    thumbRadius,
    ...theme,
  };

  let hovered = false;
  let pressed = false;

  function valueFromPosition(ex, ey) {
    if (vertical) {
      const usable = height - t.thumbRadius * 2;
      return Math.max(0, Math.min(1, 1 - (ey - t.thumbRadius) / usable));
    }
    const usable = width - t.thumbRadius * 2;
    return Math.max(0, Math.min(1, (ex - t.thumbRadius) / usable));
  }

  function thumbCenter(value) {
    if (vertical) {
      return { cx: width / 2, cy: t.thumbRadius + (1 - value) * (height - t.thumbRadius * 2) };
    }
    return { cx: t.thumbRadius + value * (width - t.thumbRadius * 2), cy: height / 2 };
  }

  return Component({
    x,
    y,
    width,
    height,

    onMouseEnter: () => { hovered = true; },

    onMouseExit: () => {
      hovered = false;
      pressed = false;
    },

    onMouseDown: (e) => {
      pressed = true;
      setParameter(parameterId, valueFromPosition(e.x, e.y));
    },

    onMouseUp: (_e) => {
      pressed = false;
    },

    onMouseDrag: (e) => {
      setParameter(parameterId, valueFromPosition(e.x, e.y));
    },
    
    onMouseWheel: (e) => {
      let value = getParameter(parameterId);
      if (Number.isNaN(value)) return;

      const trackLength = vertical ? height : width;
      value = Math.max(0, Math.min(1, value + e.deltaY / trackLength));
      setParameter(parameterId, value);
    },

    draw: (ctx) => {
      const value = getParameter(parameterId);
      if (Number.isNaN(value)) return;

      const r   = t.cornerRadius;
      const tr  = t.thumbRadius;
      const tt  = t.trackThickness;
      const { cx, cy } = thumbCenter(value);

      ctx.save();

      if (vertical) {
        const tx = (width - tt) / 2;

        // Track
        ctx.fillStyle = t.trackColor;
        ctx.beginPath();
        ctx.roundRect(tx, 0, tt, height, r);
        ctx.fill();

        // Fill (value region)
        ctx.fillStyle = pressed ? t.fillActiveColor : t.fillColor;
        ctx.beginPath();
        ctx.roundRect(tx, cy, tt, height - cy, r);
        ctx.fill();

        // Track border
        ctx.strokeStyle = t.borderColor;
        ctx.lineWidth   = 1;
        ctx.beginPath();
        ctx.roundRect(tx, 0, tt, height, r);
        ctx.stroke();
      } else {
        const ty = (height - tt) / 2;

        // Track
        ctx.fillStyle = t.trackColor;
        ctx.beginPath();
        ctx.roundRect(0, ty, width, tt, r);
        ctx.fill();

        // Fill (value region)
        ctx.fillStyle = pressed ? t.fillActiveColor : t.fillColor;
        ctx.beginPath();
        ctx.roundRect(0, ty, cx, tt, r);
        ctx.fill();

        // Track border
        ctx.strokeStyle = t.borderColor;
        ctx.lineWidth   = 1;
        ctx.beginPath();
        ctx.roundRect(0, ty, width, tt, r);
        ctx.stroke();
      }

      // Thumb
      ctx.fillStyle = pressed ? t.thumbPressColor : hovered ? t.thumbHoverColor : t.thumbColor;
      ctx.beginPath();
      ctx.arc(cx, cy, tr, 0, Math.PI * 2);
      ctx.fill();

      ctx.strokeStyle = t.thumbBorder;
      ctx.lineWidth   = 1;
      ctx.beginPath();
      ctx.arc(cx, cy, tr, 0, Math.PI * 2);
      ctx.stroke();

      ctx.restore();
    },
  });
}
