import { Component, getParameter } from "singularity";
import { Knob } from "./knob.js"
import { Basic } from "./basic.js";
import { Bloom } from "./bloom.js";
import { Test } from "./test.js";
import { Gradients } from "./gradients.js";
import { Button } from "./button.js";
import { PostEffects } from "./post_effects.js";

export default function App() {
  console.log("Hello From JavaScript");
  return Component({
    width: 800,
    height: 600,
    children: [
      Bloom({ width: 800, height: 600 }),
    ],
  });
}
