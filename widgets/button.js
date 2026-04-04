import { Widget } from "./widget.js";
import { getParameter, setParameter } from "native:parameters";

export class Button extends Widget {
    constructor(x = 0, y = 0, width = 80, height = 30, parameterId, theme = {}) {
        super(x, y, width, height);

        this.parameterId = parameterId;

        this.theme = {
            activeColor: "#00d4ff",
            inactiveColor: "#ff0000",
            activeBorder: "#00a8cc",
            inactiveBorder: "#3a3a5c",
            labelColor: "#ffffff",
            activeLabelColor: "#000000",
            fontSize: 12,
            label: "",
            ...theme
        };
    }

    onMouseDown(x, y) {
        const value = getParameter(this.parameterId);
        if (isNaN(value)) return;
        setParameter(this.parameterId, value >= 0.5 ? 0.0 : 1.0); // toggle
        this.repaint();
    }

    onMouseUp(x, y) {
    }

    paint(ctx) {
        const value = getParameter(this.parameterId);
        if (isNaN(value)) return; // Unknown parameter, don't draw
        const active = value >= 0.5;

        ctx.fillStyle = active ? this.theme.activeColor : this.theme.inactiveColor;
        ctx.fillRect(0, 0, this.width, this.height);

        ctx.strokeStyle = active ? this.theme.activeBorder : this.theme.inactiveBorder;
        ctx.lineWidth = 1;
        ctx.strokeRect(0, 0, this.width, this.height);

        if (this.theme.label) {
            ctx.font = `${this.theme.fontSize}px sans-serif`;
            ctx.textAlign = "center";
            ctx.textBaseline = "middle";
            ctx.fillStyle = active ? this.theme.activeLabelColor : this.theme.labelColor;
            ctx.fillText(this.theme.label, this.width / 2, this.height / 2 + 4);
        }
    }
}