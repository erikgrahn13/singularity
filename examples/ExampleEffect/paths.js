import { startUI, Component } from "../widgets/ui.js";

class Editor extends Component {
    constructor() {
        super();


    }

    _starPath(ctx, center_x, center_y, radius) {
        ctx.beginPath();
        const num_points = 10;
        for (let i = 0; i < num_points; ++i) {
            const angle = i / num_points * 2.0 * Math.PI;
            const r = (i % 2) ? radius : radius * 0.4;
            const x = center_x + r * Math.sin(angle);
            const y = center_y + r * Math.cos(angle);
            if (i === 0) ctx.moveTo(x, y);
            else         ctx.lineTo(x, y);
        }
        ctx.closePath();
    }

    paint(ctx) {
        ctx.fillStyle = "#222222";
        ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);

        ctx.strokeStyle = "#ff44ff";
        ctx.fillStyle   = "#ff44ff";
        ctx.lineWidth   = 2;
        ctx.lineCap     = "round";
        ctx.lineJoin    = "round";

        const w = ctx.canvas.width / 3.0;
        const h = ctx.canvas.height;
        const r = Math.min(w, h) * 0.4;

        // Left panel: filled star
        this._starPath(ctx, w * 0.5, h * 0.5, r);
        ctx.fill();

        // Middle panel: stroked star
        ctx.save();
        ctx.translate(w, 0);
        ctx.setLineDash([]);
        this._starPath(ctx, w * 0.5, h * 0.5, r);
        ctx.stroke();
        ctx.restore();

        // Right panel: animated dashed stroke
        ctx.save();
        ctx.translate(w * 2, 0);
        const segment = (r * 2 * Math.PI) / 40.0;
        ctx.setLineDash([segment]);
        ctx.lineDashOffset = ctx.time() * segment;
        this._starPath(ctx, w * 0.5, h * 0.5, r);
        ctx.stroke();
        ctx.setLineDash([]);
        ctx.restore();
    }

    resized() {

    }
}

startUI(ctx, Editor);