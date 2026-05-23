import { Component } from "singularity";

export function Button({ x = 0, y = 0, width = 100, height = 32, label = "Button", onClick = null, theme = {} }) {
  const t = {
    bgColor:          "#2a2a3a",
    bgHoverColor:     "#3a3a4e",
    bgPressColor:     "#141420",
    borderColor:      "#555577",
    borderHoverColor: "#7777aa",
    textColor:        "#ffffff",
    rippleColor:      "#ffffff",
    cornerRadius:     6,
    rippleDuration:   0.35,   // seconds
    ...theme,
  };

  let pressed   = false;
  let hovered   = false;
  let ripple    = null;  // { x, y, startTime }

  const maxRadius = Math.sqrt(width * width + height * height) * 0.6;

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
      ripple = { x: e.x, y: e.y, startTime: Date.now() / 1000 };
    },

    onMouseUp: (e) => {
      if (pressed && e.x >= 0 && e.x <= width && e.y >= 0 && e.y <= height) {
        if (typeof onClick === "function") onClick();
      }
      pressed = false;
    },

    onMouseDrag: (e) => {
      pressed = e.x >= 0 && e.x <= width && e.y >= 0 && e.y <= height;
    },

    draw: (ctx) => {
      const r    = t.cornerRadius;
      const now  = Date.now() / 1000;

      ctx.save();

      // Background
      ctx.fillStyle = pressed ? t.bgPressColor : hovered ? t.bgHoverColor : t.bgColor;
      ctx.beginPath();
      ctx.roundRect(0, 0, width, height, r);
      ctx.fill();

      // Ripple — clipped inside the button shape
      if (ripple) {
        const elapsed  = now - ripple.startTime;
        const progress = elapsed / t.rippleDuration;

        if (progress < 1.0) {
          const eased  = 1 - Math.pow(1 - progress, 3);   // ease-out cubic
          const radius = eased * maxRadius;
          const alpha  = Math.round((1 - progress) * 0.3 * 255).toString(16).padStart(2, "0");

          ctx.save();
          ctx.beginPath();
          ctx.roundRect(0, 0, width, height, r);
          ctx.clip();

          ctx.fillStyle = `${t.rippleColor}${alpha}`;
          ctx.beginPath();
          ctx.arc(ripple.x, ripple.y, radius, 0, Math.PI * 2);
          ctx.fill();
          ctx.restore();

          ctx.redraw();
        } else {
          ripple = null;
        }
      }

      // Border
      ctx.strokeStyle = hovered ? t.borderHoverColor : t.borderColor;
      ctx.lineWidth   = 1;
      ctx.beginPath();
      ctx.roundRect(0, 0, width, height, r);
      ctx.stroke();

      // Label
      ctx.fillStyle    = t.textColor;
      ctx.font         = `${Math.floor(height * 0.45)}px sans-serif`;
      ctx.textAlign    = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(label, width / 2, height / 2);

      ctx.restore();
    },
  });
}
