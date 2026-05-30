import { Component, getParameter } from "singularity";
import { Knob }   from "singularity/knob.js";
import { Button } from "singularity/button.js";
import { Slider } from "singularity/slider.js";

export default function App() {
  console.log("Hello From JavaScript");
  const width = 800;
  const height = 600;
  return Component({
    width,
    height,
    backgroundColor: "#000000",
    draw(ctx) {
      ctx.fillStyle = "#ffffff";
      ctx.font = "48px mb-forever-raw.regular.ttf";
      ctx.textAlign = "center";
      ctx.fillText("ExampleEffect", width / 2, 40);
      ctx.drawImage("./logo_transparent.png", 100, 0, 200, 200);
    },
    children: [
      Knob({ x: 0, y: 0, size: 80, parameterId: 13 }),
      Knob({ x: 0, y: 100, size: 80, parameterId: 13 }),
      Slider({x: 100, y: 20, parameterId: 13}),
      Button({x: 20, y: 200})
    ],
  });
}
