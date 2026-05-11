import { Component } from "singularity";

export function Bloom({ width = 800, height = 300 }) {
  const kNumPoints = 1200;
  const kNumDots   = 10;
  const kDotRadius = 5;

  // C++ rainbow gradient: 0xffff6666, 0xffffff66, 0xff66ff66, 0xff66ffff, 0xff6666ff, 0xffff66ff, 0xffff6666
  const kRainbow = ["#ff6666", "#ffff66", "#66ff66", "#66ffff", "#6666ff", "#ff66ff", "#ff6666"];

  let points = new Array(kNumPoints).fill(0);

  function setLinePositions(render_time) {
    const line_phase = render_time * 0.5;
    let position = 0.0;
    for (let i = 0; i < kNumPoints; ++i) {
      const t     = 1.1 * i / (kNumPoints - 1.0) - 0.05;
      const delta = Math.min(t, 1.0 - t);
      position   += 0.02 * delta * delta + 0.003;
      points[i]   = 0.5 + Math.sin((line_phase + position) * 2.0 * Math.PI) * 0.25;
    }
  }

  // Mirrors C++ computeBrush: Brush::linear(rainbow * boost, {0,0}, {width,0})
  // rainbow * boost = rainbow color at t, multiplied by white with per-position HDR
  function computeBrush(ctx, render_time) {
    const boost_time  = render_time * 0.2;
    const boost_phase = (boost_time - Math.floor(boost_time)) * 1.5 - 0.25;

    const grad = ctx.createLinearGradient(0, 0, width, 0);
    const n = kRainbow.length - 1;

    for (let i = 0; i <= 256; i++) {
      const t = i / 256;

      // Sample rainbow at t (linear interpolation between stops)
      const si       = Math.min(Math.floor(t * n), n - 1);
      const f        = t * n - si;
      const ca       = kRainbow[si],    cb = kRainbow[si + 1];
      const ra       = parseInt(ca.slice(1,3),16), ga = parseInt(ca.slice(3,5),16), ba = parseInt(ca.slice(5,7),16);
      const rb       = parseInt(cb.slice(1,3),16), gb = parseInt(cb.slice(3,5),16), bb = parseInt(cb.slice(5,7),16);
      const r        = Math.round(ra + (rb - ra) * f);
      const g        = Math.round(ga + (gb - ga) * f);
      const b        = Math.round(ba + (bb - ba) * f);
      const color    = `#${r.toString(16).padStart(2,'0')}${g.toString(16).padStart(2,'0')}${b.toString(16).padStart(2,'0')}`;

      // Boost HDR: 1 + max(0, 0.4 - 3 * |boost_phase - t|)
      const hdr = 1.0 + Math.max(0, 0.4 - 3.0 * Math.abs(boost_phase - t));

      grad.addColorStop(t, color, hdr);
    }
    return grad;
  }

  return Component({
    width,
    height,
    postEffect: { type: "bloom", size: 30, intensity: 2 },
    draw: (ctx) => {
      const render_time = typeof ctx.time === "function" ? ctx.time() : Date.now() / 1000;
      setLinePositions(render_time);

      // ExampleEditor: canvas.setColor(0xff22282d); canvas.fill(...)
      ctx.fillStyle = "#22282d";
      ctx.fillRect(0, 0, width, height);

      // brush = computeBrush(render_time) — used for both line and dots
      const brush = computeBrush(ctx, render_time);

      // Line — C++: palette()->setColor(GraphLine::LineColor, brush)
      ctx.strokeStyle = brush;
      ctx.lineWidth   = 2.5;
      ctx.beginPath();
      for (let i = 0; i < kNumPoints; ++i) {
        const x = (i / (kNumPoints - 1)) * width;
        const y = height * points[i];
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // Dots — C++: canvas.setColor(brush); canvas.circle(...)
      ctx.fillStyle  = brush;
      const center_y = height * 0.125;
      for (let i = 0; i < kNumDots; ++i) {
        const t        = (i + 1) / (kNumDots + 1.0);
        const center_x = t * width;
        ctx.beginPath();
        ctx.arc(center_x, center_y, kDotRadius, 0, Math.PI * 2);
        ctx.fill();
        ctx.beginPath();
        ctx.arc(center_x, height - center_y, kDotRadius, 0, Math.PI * 2);
        ctx.fill();
      }

      ctx.redraw();
    },
  });
}

