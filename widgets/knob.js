import { Widget } from "./widget.js";
import { getParameter, setParameter } from "native:parameters";

export class Knob extends Widget {
    constructor(x, y, size = 40, parameterId, theme = {}) {
        super(x, y, size, size);
        this.parameterId = parameterId;
        // this.value = Math.max(0, Math.min(1, value));
        this.dragging = false;
        this.theme = {
            bodyColor:  '#1e1e2e',
            trackColor: '#3a3a5c',
            valueColor: '#00d4ff',
            dotColor:   '#ffffff',
            trackWidth: size * 0.13,
            ...theme
        };
    }

    onMouseDown(x, y) {
        this.dragging = true;
        this.lastY = y;
        this.repaint();
    }

    onMouseMove(x, y) {
        if (!this.dragging) return;
        const dy = y - (this.lastY ?? y);
        this.lastY = y;
        let value = getParameter(this.parameterId);
        if (isNaN(value)) return; // Unknown parameter, do nothing
        value = Math.max(0, Math.min(1, value - dy * 0.3 / this.height));
        setParameter(this.parameterId, value);
        this.repaint();
    }

    onMouseUp(x, y) {
        this.dragging = false;
        this.lastY = undefined;
        this.repaint();
    }

    paint(ctx) {
        const size = this.width;
        const radius = size / 2;
        const cx = radius;
        const cy = radius;
        // const value = this.value;
        const value = getParameter(this.parameterId);
        if (isNaN(value)) return; // Unknown parameter, don't draw

        // Arc goes from 135° to 405° (270° sweep)
        const startAngle = 0.75 * Math.PI;
        const totalSweep = 1.5 * Math.PI;
        const valueAngle = startAngle + value * totalSweep;
        const trackRadius = radius * 0.72;

        ctx.save();

        const { bodyColor, trackColor, valueColor, dotColor, trackWidth } = this.theme;
        const tw = typeof trackWidth === 'number' ? trackWidth : radius * 0.13;

        // Body
        ctx.beginPath();
        ctx.arc(cx, cy, radius, 0, Math.PI * 2);
        ctx.fillStyle = bodyColor;
        ctx.fill();

        // Track (background arc)
        ctx.beginPath();
        ctx.arc(cx, cy, trackRadius, startAngle, startAngle + totalSweep, false);
        ctx.strokeStyle = trackColor;
        ctx.lineWidth = tw;
        ctx.lineCap = 'round';
        ctx.stroke();

        // Value arc
        ctx.beginPath();
        ctx.arc(cx, cy, trackRadius, startAngle, valueAngle, false);
        ctx.strokeStyle = valueColor;
        ctx.lineWidth = tw;
        ctx.lineCap = 'round';
        ctx.stroke();

        // Indicator dot
        const dotX = cx + Math.cos(valueAngle) * trackRadius;
        const dotY = cy + Math.sin(valueAngle) * trackRadius;
        ctx.beginPath();
        ctx.arc(dotX, dotY, radius * 0.07, 0, Math.PI * 2);
        ctx.fillStyle = dotColor;
        ctx.fill();

        ctx.restore();
    }
}