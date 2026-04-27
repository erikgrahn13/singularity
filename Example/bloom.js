import { Component } from "singularity";

export function Bloom({ width = 800, height = 300 }) {
  const kNumPoints = 1200;
  const kNumDots = 10;
  const kDotRadius = 5;

  // Store phase as a property on the component instance (if supported)
  let line_phase = 0;
  let points = new Array(kNumPoints).fill(0);
//   if(typeof ctx.time === "function")
//   {
    // console.log("canvas time found");
//   }

  function setLinePositions(render_time) {
    let position = 0.0;
    line_phase = render_time * 0.5;
    for (let i = 0; i < kNumPoints; ++i) {
      const t = 1.1 * i / (kNumPoints - 1.0) - 0.05;
      const delta = Math.min(t, 1.0 - t);
      position += 0.02 * delta * delta + 0.003;
      points[i] = 0.5 + Math.sin((line_phase + position) * 2.0 * Math.PI) * 0.25;
    }
  }

  return Component({
    width,
    height,
    postEffect: { type: "bloom", size: 30, intensity: 10 },
    draw: (ctx) => {
      // Use a time source if available, otherwise fallback to Date.now()
      const render_time = (typeof ctx.time === "function" ? ctx.time() : Date.now() / 1000);

      setLinePositions(render_time);

      // Background
      ctx.fillStyle = "#22282d";
      ctx.fillRect(0, 0, width, height);

      // Animated line
      ctx.strokeStyle = "#ff66ff";
      ctx.lineWidth = 2.5;
      ctx.beginPath();
      for (let i = 0; i < kNumPoints; ++i) {
        const x = (i / (kNumPoints - 1)) * width;
        const y = height * points[i];
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // Dots
      ctx.fillStyle = "#ffffff";
      const center_y = height * 0.125;
      for (let i = 0; i < kNumDots; ++i) {
        const t = (i + 1) / (kNumDots + 1.0);
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