/**
 * Knob widget — a rotary control, like a volume or parameter knob.
 *
 * Usage:
 *   import { Knob } from './widgets/knob.js';
 *   const knob = new Knob(75, 75, 40, 0.7);
 *   knob.draw();
 */
export class Knob {
    /**
     * @param {number} x       - Center X
     * @param {number} y       - Center Y
     * @param {number} radius  - Outer radius
     * @param {number} value   - Normalized value 0..1  (default 0.5)
     * @param {object} theme   - Optional visual overrides
     */
    constructor(x, y, radius, value = 0.5, theme = {}) {
        this.x = x;
        this.y = y;
        this.radius = radius;
        this.value = Math.max(0, Math.min(1, value));
        this.theme = {
            bodyColor:  '#1e1e2e',
            trackColor: '#3a3a5c',
            valueColor: '#00d4ff',
            dotColor:   '#ffffff',
            trackWidth: radius * 0.13,  // relative to radius by default
            ...theme
        };
        _registerWidget();
        this.draw();
    }

    draw() {
        const { x, y, radius, value } = this;

        // Arc goes from 225° (bottom-left) clockwise 270° to 315° (bottom-right)
        const startAngle = 0.75 * Math.PI;   // 135° in canvas coords
        const totalSweep = 1.5 * Math.PI;    // 270°
        const valueAngle = startAngle + value * totalSweep;
        const trackRadius = radius * 0.72;

        ctx.save();

        const { bodyColor, trackColor, valueColor, dotColor, trackWidth } = this.theme;
        const tw = typeof trackWidth === 'number' ? trackWidth : radius * 0.13;

        // Body
        ctx.beginPath();
        ctx.arc(x, y, radius, 0, Math.PI * 2);
        ctx.fillStyle = bodyColor;
        ctx.fill();

        // Track (background arc)
        ctx.beginPath();
        ctx.arc(x, y, trackRadius, startAngle, startAngle + totalSweep, false);
        ctx.strokeStyle = trackColor;
        ctx.lineWidth = tw;
        ctx.lineCap = 'round';
        ctx.stroke();

        // Value arc
        ctx.beginPath();
        ctx.arc(x, y, trackRadius, startAngle, valueAngle, false);
        ctx.strokeStyle = valueColor;
        ctx.lineWidth = tw;
        ctx.lineCap = 'round';
        ctx.stroke();

        // Indicator dot
        const dotX = x + Math.cos(valueAngle) * trackRadius;
        const dotY = y + Math.sin(valueAngle) * trackRadius;
        ctx.beginPath();
        ctx.arc(dotX, dotY, radius * 0.07, 0, Math.PI * 2);
        ctx.fillStyle = dotColor;
        ctx.fill();

        ctx.restore();
    }
}
