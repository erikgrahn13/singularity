import { Component } from "singularity"

export default function App() {
    console.log("Hello From JavaScript");
  return Component({
    width: 800,
    height: 600,
    children: [
      Component({ 
        x: 20, 
        y: 30, 
        width: 200, 
        height: 100,
        draw: (ctx) => {
            console.log("draw1 called");
            ctx.fillStyle = "#ff0000";
            ctx.fillRect(0, 0, 200, 100);
            ctx.clearRect(10, 10, 50, 50);
        }
      }),
      Component({ 
        x: 320, 
        y: 30, 
        width: 250, 
        height: 100,
        draw: (ctx) => {
            console.log("draw2 called");
            ctx.strokeStyle = "#ff0000";
            ctx.strokeRect(0, 0, 150, 50)
        }
      })
    ]
  })
}