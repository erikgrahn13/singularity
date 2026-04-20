import { startUI, Component } from "../widgets/ui.js";

class Editor extends Component {
    constructor() {
        super();


    }

    paint(ctx) {
        ctx.fillStyle = "#222233";
        ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);


    }

    resized() {

    }
}

startUI(ctx, Editor);