import { startUI, Component } from "./ui.js";
import { Button } from "./button.js";
import { Knob } from "./knob.js";
import { StepSelector } from "./stepselector.js";
import { Dropdown } from "./dropdown.js";
import { getAudioBackends, getStringList } from "native:audio";

class Editor extends Component {
    constructor() {
        super();

        const backends = getAudioBackends();

        this.driverDropdown = new Dropdown(0, 0, 300, 30,
            backends,
            (index, label) => {
                this.deviceDropdown.setItems(getStringList("devices:" + label));
            }
        );
        this.addChild(this.driverDropdown);

        this.deviceDropdown = new Dropdown(0, 0, 300, 30,
            getStringList("devices:" + (backends[0] ?? "")),
            (index, label) => { /* handle selection */ }
        );
        this.addChild(this.deviceDropdown);
     }

    paint(ctx) {
        ctx.fillStyle = "#ff000000";
        ctx.fillRect(0, 0, this.width, this.height);

        ctx.fillStyle = "#ffffff";
        ctx.font = "14px serif";

        ctx.textAlign = "right";
        ctx.fillText("Audio device type:", 150, 50);
        ctx.fillText("Device:", 150, 100);
        ctx.fillText("Active output channels:", 150, 150);
        ctx.fillText("Active input channels:", 150, 200);
        ctx.fillText("Sample rate:", 150, 250);
        ctx.fillText("Audio buffer size:", 150, 300);
    }

    resized() {
        this.driverDropdown?.setBounds(160, 30, 200, 30);
        this.deviceDropdown?.setBounds(160, 80, 200, 30);
    }
}

startUI(ctx, Editor, { showOverlays: false });