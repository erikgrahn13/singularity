import { startUI, Component } from "./widgets/ui.js";
import { Button } from "./widgets/button.js";
import { Knob } from "./widgets/knob.js";
import { StepSelector } from "./widgets/stepselector.js";

class Editor extends Component {
    constructor() {
        super();

        this.button = new Button(0, 0, 120, 60, 7, { label: "Bypass" });
        this.knob = new Knob(250, 100, 60, 13);
        this.waveform = new StepSelector(0, 0, 240, 30, 15, ["Sine", "Square", "Saw", "Tri"]);

        this.addChild(this.button);
        this.addChild(this.knob);
        this.addChild(this.waveform);
    }

    paint(ctx) {
        ctx.fillStyle = "#ff000000";
        ctx.fillRect(0, 0, this.width, this.height);
    }

    resized() {
        this.button.setBounds(100, 100, 120, 60);
        this.knob.setBounds(250, 100, 60, 60);
        this.waveform.setBounds(100, 180, 240, 30);
    }
}

startUI(ctx, Editor);