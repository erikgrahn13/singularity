// Examples from MDN for all the drawing primities
import {Button} from "./widgets/button.js";

//https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/fillRect
ctx.fillStyle = "#00ff00";
ctx.fillText("fillRect", 20, 10);
ctx.translate(0, 0);
ctx.fillRect(20, 10, 150, 100);

ctx.translate(0, 140);
ctx.fillText("strokeRect", 20, 10);

//https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/strokeRect
ctx.strokeStyle = "#00ff00";
ctx.strokeRect(20, 10, 160, 100);

ctx.translate(0, 140);
ctx.fillText("roundRect", 20, 10);
ctx.strokeStyle = "#001aff";
ctx.beginPath();
ctx.roundRect(10, 10, 150, 100, [40]);
ctx.stroke();

const button = new Button(ctx, 100, 200, 50, 50);