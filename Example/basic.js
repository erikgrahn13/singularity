import { startUI, Component } from "../widgets/ui.js";

class Editor extends Component {
    constructor() {
        super();


    }

    paint(ctx) {
        ctx.fillStyle = "#000066";
        ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);

        const r = ctx.canvas.height * 0.1;
        const x = ctx.canvas.width * 0.5;
        const y = ctx.canvas.height * 0.5;
        ctx.fillStyle = "#00ffff";
        ctx.circle(x, y, r);
    }

    resized() {

    }
}

startUI(ctx, Editor);