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
// ctx.strokeStyle = "#ff0000ba"
// ctx.lineWidth = 10;
// ctx.lineCap = 'square'
// ctx.lineJoin = 'round'

// ctx.fillStyle = "#1eff00ba"

// // ctx.translate(20, 20)
// // ctx.rotate(10)
// ctx.beginPath()
// ctx.lineWidth = 10
// ctx.shadowColor = "#0011ff"
// ctx.shadowOffsetY = 5;
// ctx.shadowOffsetX = 5;
// ctx.roundRect(0, 0, 300,300,20)

// ctx.shadowBlur = 25
// ctx.fill()

// ctx.resetTransform()

// ctx.strokeStyle = "#000000ba"

// // TEST CODE BELOW

const gradient = ctx.createLinearGradient(20, 0, 220, 0);

// Add three color stops
gradient.addColorStop(0, "#1f8628");
gradient.addColorStop(0.5, "#00ffd5");
gradient.addColorStop(1, "#1f8628");

// Set the fill style and draw a rectangle
ctx.fillStyle = gradient;
ctx.fillRect(20, 20, 200, 100);


const gradient2 = ctx.createLinearGradient(20, 0, 220, 0);


// Add three color stops
gradient2.addColorStop(0, "#ff0000");
gradient2.addColorStop(0.5, "#ff00ff");
gradient2.addColorStop(1, "#ff0000");

// Set the fill style and draw a rectangle
ctx.fillStyle = gradient2;
ctx.fillRect(20, 220, 200, 100);


const gradient3 = ctx.createRadialGradient(100, 500, 10, 100, 500, 80);

// Add three color stops
gradient3.addColorStop(0, "#ff00ff");
gradient3.addColorStop(0.9, "#ffffff");
gradient3.addColorStop(1, "#1f8628");

// Set the fill style and draw a rectangle
ctx.fillStyle = gradient3;
ctx.fillRect(20, 420, 160, 160);

// const img = { _id: 0 };  // id 0 = first registered image

// ctx.drawImage('logo', 50, 50, 200, 200);

// // TEST CODE ABOVE





// console.log(text.width);