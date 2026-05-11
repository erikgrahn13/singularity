import { Component } from "singularity";

export function Test({x, y, width, height }) {
    console.log("hello from test.js")
  return Component({
    y,
    x,
    width,
    height,
    draw: (ctx) => {
      const gradient = ctx.createLinearGradient(x, y, width, height);

      // Add three color stops
      gradient.addColorStop(0, "#00ff0d");
      gradient.addColorStop(0.5, "#00ffdd");
      gradient.addColorStop(1, "#ff0000");

      // Set the fill style and draw a rectangle
      ctx.fillStyle = gradient;
      ctx.fillRect(x, y, width, height);
    },
  });
}
