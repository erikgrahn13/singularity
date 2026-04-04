import { Widget } from "./widget.js";
import { getParameter, setParameter } from "native:parameters";

/**
 * StepSelector — a segmented control for stepped parameters.
 * Each segment represents one step. Clicking a segment sets the parameter
 * to that step's normalized value.
 *
 * Usage:
 *   new StepSelector(x, y, width, height, parameterId, ["Sine", "Square", "Saw", "Tri"])
 */
export class StepSelector extends Widget {
    constructor(x = 0, y = 0, width = 200, height = 30, parameterId, labels = [], theme = {}) {
        super(x, y, width, height);
        this.parameterId = parameterId;
        this.labels = labels;

        if (isNaN(getParameter(this.parameterId))) {
            console.log(`StepSelector: parameter id ${parameterId} is not registered`);
        }

        this.theme = {
            activeColor:    "#00d4ff",
            inactiveColor:  "#1e1e2e",
            activeBorder:   "#00a8cc",
            inactiveBorder: "#3a3a5c",
            labelColor:     "#ffffff",
            activeLabelColor: "#000000",
            fontSize:       12,
            ...theme
        };
    }

    // Convert normalized parameter value (0..1) to step index
    valueToStep(value) {
        const count = this.labels.length;
        if (count <= 1) return 0;
        return Math.round(value * (count - 1));
    }

    // Convert step index to normalized parameter value (0..1)
    stepToValue(step) {
        const count = this.labels.length;
        if (count <= 1) return 0;
        return step / (count - 1);
    }

    onMouseDown(x, y) {
        const value = getParameter(this.parameterId);
        if (isNaN(value)) return;

        const count = this.labels.length;
        const segmentWidth = this.width / count;
        const clickedStep = Math.floor(x / segmentWidth);
        if (clickedStep < 0 || clickedStep >= count) return;

        setParameter(this.parameterId, this.stepToValue(clickedStep));
        this.repaint();
    }

    paint(ctx) {
        const value = getParameter(this.parameterId);
        if (isNaN(value)) return; // Unknown parameter, don't draw

        const activeStep = this.valueToStep(value);
        const count = this.labels.length;
        const segmentWidth = this.width / count;

        for (let i = 0; i < count; i++) {
            const isActive = i === activeStep;
            const segX = i * segmentWidth;

            // Background
            ctx.fillStyle = isActive ? this.theme.activeColor : this.theme.inactiveColor;
            ctx.fillRect(segX, 0, segmentWidth, this.height);

            // Border
            ctx.strokeStyle = isActive ? this.theme.activeBorder : this.theme.inactiveBorder;
            ctx.lineWidth = 1;
            ctx.strokeRect(segX, 0, segmentWidth, this.height);

            // Label
            if (this.labels[i]) {
                ctx.font = `${this.theme.fontSize}px sans-serif`;
                ctx.textAlign = "center";
                ctx.textBaseline = "middle";
                ctx.fillStyle = isActive ? this.theme.activeLabelColor : this.theme.labelColor;
                ctx.fillText(this.labels[i], segX + segmentWidth / 2, this.height / 2);
            }
        }
    }
}
