import { Component } from "singularity"

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
            ctx.fillStyle = "#003cff";
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
            console.log("dragging", e.x);
        }
      }),
      Component({ 
        x: 320, 
        y: 30, 
        width: 60, 
        height: 60,
        draw: (ctx) => {
            const size = 60;
            const radius = size / 2;
            const cx = radius;
            const cy = radius;
            // const value = this.value;
            const value = 0.5;
            // if (isNaN(value)) return; // Unknown parameter, don't draw

            // Arc goes from 135° to 405° (270° sweep)
            const startAngle = 0.75 * Math.PI;
            const totalSweep = 1.5 * Math.PI;
            const valueAngle = startAngle + value * totalSweep;
            const trackRadius = radius * 0.72;

            ctx.save();

            // const { bodyColor, trackColor, valueColor, dotColor, trackWidth } = this.theme;
            // const tw = typeof trackWidth === 'number' ? trackWidth : radius * 0.13;
            const tw = 60 * 0.13;
            // Body
            ctx.beginPath();
            ctx.arc(cx, cy, radius, 0, Math.PI * 2);
            ctx.fillStyle = '#1e1e2e';
            ctx.fill();

            // Track (background arc)
            ctx.beginPath();
            ctx.arc(cx, cy, trackRadius, startAngle, startAngle + totalSweep, false);
            ctx.strokeStyle = '#3a3a5c';
            ctx.lineWidth = tw;
            ctx.lineCap = 'round';
            ctx.stroke();

            // Value arc
            ctx.beginPath();
            ctx.arc(cx, cy, trackRadius, startAngle, valueAngle, false);
            ctx.strokeStyle = '#00d4ff';
            ctx.lineWidth = tw;
            ctx.lineCap = 'round';
            ctx.stroke();

            // Indicator dot
            const dotX = cx + Math.cos(valueAngle) * trackRadius;
            const dotY = cy + Math.sin(valueAngle) * trackRadius;
            ctx.beginPath();
            ctx.arc(dotX, dotY, radius * 0.07, 0, Math.PI * 2);
            ctx.fillStyle = '#ffffff';
            ctx.fill();

            ctx.restore();


        }
      })
    ]
  })
}