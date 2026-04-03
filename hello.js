import { startUI, Component } from "./widgets/ui.js";
import { Button } from "./widgets/button.js";
import { Knob } from "./widgets/knob.js"; // Import your Knob

class Editor extends Component {
    constructor() {
        super();

        this.button = new Button(0, 0, 120, 60, { label: "Click me!" });
        this.knob = new Knob(250, 100, 60, 13); // x, y, size, value

        this.addChild(this.button);
        this.addChild(this.knob);
    }

    paint(ctx) {
        ctx.fillStyle = "#ff000000";
        ctx.fillRect(0, 0, this.width, this.height);
    }

    resized() {
        this.button.setBounds(100, 100, 120, 60);
        this.knob.setBounds(250, 100, 60, 60); // Position and size the knob
    }
}

startUI(ctx, Editor);