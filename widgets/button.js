/**
 * Button widget — a simple toggle/momentary button.
 *
 * Usage:
 *   import { Button } from './widgets/button.js';
 *   const btn = new Button(10, 170, 80, 22, true);
 *   btn.draw();
 */
export class Button {
    /**
     * @param {number}  x      - Top-left X
     * @param {number}  y      - Top-left Y
     * @param {number}  width
     * @param {number}  height
     * @param {boolean} active - Whether the button is pressed / on
     * @param {object}  theme  - Optional visual overrides
     */
    constructor(x, y, width, height, active = false, theme = {}) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.active = active;
        this.theme = {
            activeColor:      '#00d4ff',
            inactiveColor:    '#1e1e2e',
            activeBorder:     '#00a8cc',
            inactiveBorder:   '#3a3a5c',
            activeHighlight:  '#80efff55',
            inactiveHighlight:'#ffffff15',
            labelColor:       '#ffffff',
            activeLabelColor: '#000000',
            fontSize:         10,
            label:            '',
            ...theme
        };
        this.draw();
    }

    draw() {
        const { x, y, width, height, active } = this;
        const r = height * 0.25; // corner radius approximated via arc

        ctx.save();

        const { activeColor, inactiveColor, activeBorder, inactiveBorder,
                activeHighlight, inactiveHighlight } = this.theme;

        // Body
        ctx.fillStyle = active ? activeColor : inactiveColor;
        ctx.fillRect(x, y, width, height);

        // Border
        ctx.strokeStyle = active ? activeBorder : inactiveBorder;
        ctx.lineWidth = 1;
        ctx.strokeRect(x, y, width, height);

        // Inner highlight line at top edge
        ctx.beginPath();
        ctx.moveTo(x + 1, y + 1);
        ctx.lineTo(x + width - 1, y + 1);
        ctx.strokeStyle = active ? activeHighlight : inactiveHighlight;
        ctx.lineWidth = 1;
        ctx.stroke();

        // Label
        if (this.theme.label) {
            ctx.fontSize = this.theme.fontSize;
            ctx.fillStyle = active ? this.theme.activeLabelColor : this.theme.labelColor;
            ctx.fillText(this.theme.label,
                x + width  / 2 - this.theme.label.length * this.theme.fontSize * 0.3,
                y + height / 2 + this.theme.fontSize * 0.35);
        }

        ctx.restore();
    }
}
