import { Component, getParameter } from "singularity";
import { Knob } from "singularity/knob.js";
import { Button } from "singularity/button.js";
import { Slider } from "singularity/slider.js";
import { LevelMeter } from "singularity/levelmeter.js";

export default function App() {
  console.log("Hello From JavaScript");
  const width = 800;
  const height = 600;

  return Component({
    width,
    height,
    backgroundColor: "#1b1028",
    draw(ctx) {
      const volume = getParameter(13);
      const character = getParameter(14);
      const outputLevel = getParameter(15);

      ctx.fillStyle = "#ffffff";
      ctx.fillRect(20, 10, 220, 120);

      ctx.fillStyle = "#1b1028";
      ctx.font = "16px sans-serif";
      ctx.fillText(`Volume: ${Number.isNaN(volume) ? "n/a" : volume.toFixed(2)} linear`, 36, 42);
      ctx.fillText(`Character index: ${Number.isNaN(character) ? "n/a" : character.toFixed(0)}`, 36, 68);
      ctx.fillText(`Output level: ${Number.isNaN(outputLevel) ? "n/a" : outputLevel.toFixed(2)}`, 36, 94);
    },
    children: [
      Knob({ x: 280, y: 40, size: 110, parameterId: 13 }),
      Slider({ x: 430, y: 80, width: 220, height: 28, parameterId: 13 }),
      Button({ x: 36, y: 150, width: 120, height: 40 }),
      LevelMeter({ x: 180, y: 150, width: 30, height: 180, parameterId: 15 }),
    ],
  });
}
