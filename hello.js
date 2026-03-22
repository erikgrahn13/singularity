// import { Knob }   from './widgets/knob.js';
// import { Slider } from './widgets/slider.js';
// import { Button } from './widgets/button.js';

// const theme = {
//     valueColor: '#ff6b35',
//     trackColor: '#2a2a2a',
//     bodyColor:  '#141414',
//     thumbColor: '#ff6b35',
//     fillColor:  '#ff6b3533',
//     activeColor:'#ff6b35',
// };

// // --- Layout helpers ---
// const W = 300, H = 200;

// const knobRow   = { x: 30, y: 80,  spacing: 60 };
// const sliderCol = { x: 255, y: 20 };
// const sliderRow = { x: 20,  y: 148, width: 180 };
// const buttonRow = { x: 20,  y: 172, spacing: 65 };

// // Background
// ctx.fillStyle = '#0d0d1a';
// ctx.fillRect(0, 0, W, H);

// // Knobs
// new Knob(knobRow.x + 0 * knobRow.spacing, knobRow.y, 35, 0.3, theme);
// new Knob(knobRow.x + 1 * knobRow.spacing, knobRow.y, 35, 0.7, theme);
// new Knob(knobRow.x + 2 * knobRow.spacing, knobRow.y, 35, 1.0, { ...theme, valueColor: '#fd0000' });

// // Sliders
// new Slider(sliderCol.x, sliderCol.y, 14, 120, 0.6, true,  theme);
// new Slider(sliderRow.x, sliderRow.y, sliderRow.width, 12, 0.4, false, theme);

// // Buttons
// new Button(buttonRow.x + 0 * buttonRow.spacing, buttonRow.y, 55, 18, true,  { ...theme, label: 'ON' });
// new Button(buttonRow.x + 1 * buttonRow.spacing, buttonRow.y, 55, 18, false, { ...theme, label: 'OFF' });
// ctx.fillStyle("#ffaabb")
// hello()
// ctx.globalAlpha = 0.1

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
ctx.strokeStyle = "#ff0000ba"
// ctx.lineWidth = 10;
// ctx.lineCap = 'square'
// ctx.lineJoin = 'round'

ctx.fillStyle = "#66ff00ba"

ctx.beginPath()
ctx.roundRect(0, 0, 300,300,20)
ctx.fill()

ctx.strokeStyle = "#000000ba"

// TEST CODE BELOW

// Define the points as {x, y}
let start = { x: 50, y: 20 };
let cp1 = { x: 230, y: 30 };
let cp2 = { x: 150, y: 80 };
let end = { x: 250, y: 100 };

// Cubic Bézier curve
ctx.beginPath();
ctx.moveTo(start.x, start.y);
ctx.bezierCurveTo(cp1.x, cp1.y, cp2.x, cp2.y, end.x, end.y);
ctx.stroke();

// Start and end points
ctx.fillStyle = "blue";
ctx.beginPath();
ctx.arc(start.x, start.y, 5, 0, 2 * Math.PI); // Start point
ctx.arc(end.x, end.y, 5, 0, 2 * Math.PI); // End point
ctx.fill();

// Control points
ctx.fillStyle = "red";
ctx.beginPath();
ctx.arc(cp1.x, cp1.y, 5, 0, 2 * Math.PI); // Control point one
ctx.arc(cp2.x, cp2.y, 5, 0, 2 * Math.PI); // Control point two
ctx.fill();

// TEST CODE ABOVE

ctx.fillStyle = "#00ff0dba"
ctx.strokeStyle = "#000000ba"
// ctx.lineWidth = 10;
ctx.lineCap = 'round'
ctx.lineJoin = 'bevel'
//ctx.beginPath()
//ctx.arc(100, 75, 50, 140* Math.PI/180, 400*Math.PI/180);
//ctx.stroke()
// ctx.fillStyle = "#1010c0"
// ctx.font = "16px"
// ctx.textAlign = "end"
// ctx.strokeText("Erik Grahn2", 100, 100)
// let text = ctx.measureText("Hello world");
const baselines = [
  "top",
  "hanging",
  "middle",
  "alphabetic",
  "ideographic",
  "bottom",
];
ctx.font = "36px serif";
ctx.strokeStyle = "#ff0000";

baselines.forEach((baseline, index) => {
  ctx.textBaseline = baseline;
  const y = 75 + index * 75;
  ctx.beginPath();
  ctx.moveTo(0, y + 0.5);
  ctx.lineTo(550, y + 0.5);
  ctx.stroke();
  ctx.fillText(`Abcdefghijklmnop (${baseline})`, 0, y);
});



// console.log(text.width);