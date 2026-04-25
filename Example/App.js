import { Component, getParameter } from "singularity";
import { Knob } from "./knob.js"

export default function App() {
  console.log("Hello From JavaScript");
  return Component({
    width: 600,
    height: 400,
    children: [
      Component({
        x: 20,
        y: 30,
        width: 200,
        height: 100,
        draw: (ctx) => {
          console.log("draw1 called");
          ctx.fillStyle = "#00ffd5";
          ctx.fillRect(0, 0, 200, 100);
          ctx.clearRect(10, 10, 50, 50);
        },
        onMouseDown: (e) => {
          console.log("onMouseDown");
        },
        onMouseUp: (e) => {
          console.log("onMouseUp");
        },
        onMouseDrag: (e) => {
          const dy = e.y - lastY;
          lastY = e.y;

          let value = getParameter(parameterId);
          value = Math.max(0, Math.min(1, value - (dy * 0.3) / height));

          setParameter(parameterId, value);
        },
      }),
      Component({
        x: 320,
        y: 30,
        width: 120,
        height: 120,
        draw: (ctx) => {
            ctx.fillStyle = "#00ffd0"
            ctx.fillRect(0, 0, 120, 120)

            ctx.beginPath()
            ctx.arc(60, 60, 40, 0, Math.PI * 2)
            ctx.strokeStyle = "#00d4ff"
            ctx.lineWidth = 8
            ctx.stroke()

        },
      }),
      Knob({
        x: 40,
        y: 40,
        size: 80,
        parameterId: 13
      })
    ],
  });
}
