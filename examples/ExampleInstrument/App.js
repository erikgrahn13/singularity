import { Component, getParameter } from "singularity";
import { Knob }   from "singularity/knob.js";
import { Button } from "singularity/button.js";
import { Slider } from "singularity/slider.js";
import { ScrollView } from "singularity/scrollview.js";

export default function App() {
  console.log("Hello From JavaScript");
  const width = 800;
  const height = 600;
  const items = [
    "Kick",
    "Snare",
    "Hi-hat Closed",
    "Hi-hat Open",
    "Clap",
    "Tom Low",
    "Tom Mid",
    "Tom High",
    "Crash",
    "Ride",
    "Shaker",
    "Cowbell",
    "Perc 1",
    "Perc 2",
    "FX Sweep",
    "Vocal Chop",
  ];

  return Component({
    width,
    height,
    backgroundColor: "#000000",
    draw(ctx) {
      ctx.fillStyle = "#ffffff";
      ctx.font = "24px sans-serif";
      ctx.textAlign = "center"
      ctx.fillText("ExampleInstrument", width / 2, 40);
    },
    children: [
      Knob({ x: 0, y: 0, size: 80, parameterId: 13 }),
      Knob({ x: 0, y: 100, size: 80, parameterId: 13 }),
      Slider({x: 100, y: 20, parameterId: 13}),
      Button({x: 20, y: 200}),
      ScrollView({
        x: 260,
        y: 100,
        width: 250,
        height: 320,
        items,
      }),
    ],
  });
}
