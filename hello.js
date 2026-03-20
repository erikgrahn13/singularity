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
ctx.fillStyle = "#ff0000ba"
ctx.fillRect(0, 0, 300,300)

ctx.fillStyle = "#00ff0dba"
ctx.strokeStyle = "#1eff00ba"
ctx.lineWidth = 10;
ctx.lineCap = 'butt'
ctx.beginPath()
ctx.arc(100, 75, 50, 0, 2* Math.PI -1);
ctx.stroke()
