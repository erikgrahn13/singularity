import { Component, getParameter } from "singularity";
import { Knob } from "./knob.js"
import { Button } from "./button.js";
import { Slider } from "./slider.js"

export default function App() {
  console.log("Hello From JavaScript");
  return Component({
    width: 800,
    height: 600,
    backgroundColor: "#000000",
    children: [
      Knob({ x: 0, y: 0, size: 80, parameterId: 13 }),
      Knob({ x: 0, y: 100, size: 80, parameterId: 13 }),
      Slider({x: 100, y: 20, parameterId: 13}),
      Button({x: 20, y: 200})
    ],
  });
}
