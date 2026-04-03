import { Widget } from "./widget.js";

export class Button extends Widget {
    constructor(x = 0, y = 0, width = 80, height = 30, theme = {}) {
        super(x, y, width, height);

        this.active = false;
        this.dragging = false;
        this.dragOffsetX = 0;
        this.dragOffsetY = 0;

        this.theme = {
            activeColor: "#00d4ff",
            inactiveColor: "#00ff11",
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
        console.log("Button down");

        this.active = true;
        this.repaint();
    }

    onMouseUp(x, y) {
        console.log("Button clicked");
        this.active = false;
        this.repaint();
    }

    paint(ctx) {
        ctx.fillStyle = this.active ? this.theme.activeColor : this.theme.inactiveColor;
        ctx.fillRect(0, 0, this.width, this.height);

        ctx.strokeStyle = this.active ? this.theme.activeBorder : this.theme.inactiveBorder;
        ctx.lineWidth = 1;
        ctx.strokeRect(0, 0, this.width, this.height);

        if (this.theme.label) {
            ctx.font = `${this.theme.fontSize}px sans-serif`;
            ctx.textAlign = "center";
            ctx.textBaseline = "middle";
            ctx.fillStyle = this.active ? this.theme.activeLabelColor : this.theme.labelColor;
            ctx.fillText(this.theme.label, this.width / 2, this.height / 2 + 4);
        }
    }
}