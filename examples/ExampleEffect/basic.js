import { Component } from "singularity";

export function Basic({width, height}) {
  console.log("erik234", width, height);
  return Component({
    y: 0,
    x: 0,
    width,
    height,
    draw: (ctx) => {
      // background
      ctx.fillStyle = "#000066";
      ctx.fillRect(0, 0, width, height);

      // circle
      const circleRadius = height * 0.1;

      const x = width * 0.5 - circleRadius;
      const y = height * 0.5 - circleRadius;

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
