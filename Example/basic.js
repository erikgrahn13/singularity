import { Component } from "singularity";

export function Basic() {
  console.log("erik234", window.width, window.height);
  return Component({
    y: 0,
    x: 0,
    width: window.width,
    height: window.height,
    draw: (ctx) => {
      // background
      ctx.fillStyle = "#000066";
      ctx.fillRect(0, 0, window.width, window.height);

      // circle
      const circleRadius = window.height * 0.1;

      const x = window.width * 0.5 - circleRadius;
      const y = window.height * 0.5 - circleRadius;

      // convert to center
      const cx = x + circleRadius;
      const cy = y + circleRadius;

      ctx.beginPath();
      ctx.arc(cx, cy, circleRadius, 0, Math.PI * 2);
      ctx.fillStyle = "#00ffff";
      ctx.fill();
    },
  });
}
