/**
 * Slider widget — a horizontal or vertical fader.
 *
 * Usage:
 *   import { Slider } from './widgets/slider.js';
 *   const slider = new Slider(10, 120, 120, 16, 0.6);
 *   slider.draw();
 */
export class Slider {
    /**
     * @param {number}  x        - Top-left X
     * @param {number}  y        - Top-left Y
     * @param {number}  width    - Total track width
     * @param {number}  height   - Total track height
     * @param {number}  value    - Normalized value 0..1  (default 0.5)
     * @param {boolean} vertical - true for vertical fader
     * @param {object}  theme    - Optional visual overrides
     */
    constructor(x, y, width, height, value = 0.5, vertical = false, theme = {}) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.value = Math.max(0, Math.min(1, value));
        this.vertical = vertical;
        this.theme = {
            trackColor:  '#1e1e2e',
            fillColor:   '#00d4ff33',
            borderColor: '#3a3a5c',
            thumbColor:  '#00d4ff',
            thumbBorder: '#ffffff44',
            ...theme
        };
        this.draw();
    }

    draw() {
        const { x, y, width, height, value, vertical } = this;
        const thumbR = vertical ? width * 0.55 : height * 0.55;

        ctx.save();

        const { trackColor, fillColor, borderColor, thumbColor, thumbBorder } = this.theme;

        // Track
        ctx.fillStyle = trackColor;
        ctx.fillRect(x, y, width, height);

        // Fill
        ctx.fillStyle = fillColor;
        if (vertical) {
            const fillH = height * value;
            ctx.fillRect(x, y + height - fillH, width, fillH);
        } else {
            ctx.fillRect(x, y, width * value, height);
        }

        // Track border
        ctx.strokeStyle = borderColor;
        ctx.lineWidth = 1;
        ctx.strokeRect(x, y, width, height);

        // Thumb
        ctx.fillStyle = thumbColor;
        ctx.beginPath();
        if (vertical) {
            ctx.arc(x + width / 2, y + height - height * value, thumbR, 0, Math.PI * 2);
        } else {
            ctx.arc(x + width * value, y + height / 2, thumbR, 0, Math.PI * 2);
        }
        ctx.fill();

        // Thumb highlight
        ctx.strokeStyle = thumbBorder;
        ctx.lineWidth = 1;
        ctx.stroke();

        ctx.restore();
    }
}
