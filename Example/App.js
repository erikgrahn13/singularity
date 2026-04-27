import { Component, getParameter } from "singularity";
import { Knob } from "./knob.js"
import { Basic } from "./basic.js";
import { Bloom } from "./bloom.js";

export default function App() {
  console.log("Hello From JavaScript");
  return Component({
    width: 800,
    height: 600,
    children: [
      // Basic({
      //   width: 800,
      //   height: 600,
      // })
      Bloom({
        width: 800,
        height: 300,
      })
    ],
  });
}
