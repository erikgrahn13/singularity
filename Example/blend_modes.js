import { startUI, Component } from "../widgets/ui.js";

// Draws overlapping red/green/blue circles in a Venn diagram pattern,
// centered within the given width/height region.
function drawRgbCircles(ctx, width, height) {
    const kCircleRadiusRatio      = 0.2;
    const kVennRadiusRatio        = 0.13;
    const k60DegreeTriangleRatio  = 0.866025403784;

    const minDim       = Math.min(width, height);
    const circleRadius = kCircleRadiusRatio * minDim;
    const vennRadius   = kVennRadiusRatio * minDim;
    const vennOffset   = k60DegreeTriangleRatio * vennRadius;

    // C++ visage circle(topLeftX, topLeftY, diameter), ours: circle(cx, cy, radius)
    const startX = width  / 2 - circleRadius;
    const startY = height / 2 - circleRadius;

    ctx.fillStyle = "#ff0000";
    ctx.circle(startX + circleRadius, startY + vennRadius + circleRadius, circleRadius);

    ctx.fillStyle = "#00ff00";
    ctx.circle(startX - vennOffset + circleRadius, startY - vennRadius * 0.5 + circleRadius, circleRadius);

    ctx.fillStyle = "#0000ff";
    ctx.circle(startX + vennOffset + circleRadius, startY - vennRadius * 0.5 + circleRadius, circleRadius);
}

class Editor extends Component {
    constructor() {
        super();
    }

    paint(ctx) {
        const W = ctx.canvas.width;
        const H = ctx.canvas.height;
        const kColumns = 12;

        const cx = Math.floor(W / 2);
        const cy = Math.floor(H / 2);

        const qw1 = cx,     qh1 = cy;      // top-left  (Additive)
        const qw2 = W - cx, qh2 = cy;      // top-right (Subtractive)
        const qw3 = cx,     qh3 = H - cy;  // bot-left  (Masked)
        const qw4 = W - cx, qh4 = H - cy;  // bot-right (Grouped Transparency)

        ctx.font         = "16px";
        ctx.textAlign    = "center";
        ctx.textBaseline = "middle";

        // --- Main background ---
        ctx.fillStyle = "#222026";
        ctx.fillRect(0, 0, W, H);

        // Alternating columns behind the Grouped Transparency quadrant
        {
            let lastX = cx;
            let c1 = "#666666", c2 = "#888888";
            for (let i = 0; i < kColumns; i++) {
                const x = cx + Math.floor(qw4 * (i + 1) / kColumns);
                ctx.fillStyle = c1;
                [c1, c2] = [c2, c1];
                ctx.fillRect(lastX, cy, x - lastX, qh4);
                lastX = x;
            }
        }

        // === Top-left: Additive blend ===
        ctx.save();
        ctx.translate(0, 0);
        ctx.setBlendMode("add");
        ctx.fillStyle = "#ffffff";
        ctx.fillText("Additive", qw1 / 2, qh1 * 0.1);
        drawRgbCircles(ctx, qw1, qh1);
        ctx.restore();

        // === Top-right: Subtractive blend ===
        ctx.save();
        ctx.translate(cx, 0);
        ctx.setBlendMode("alpha");
        ctx.fillStyle = "#eeeeee";
        ctx.fillRect(0, 0, qw2, qh2);
        ctx.setBlendMode("sub");
        ctx.fillStyle = "#ffffff";
        ctx.fillText("Subtractive", qw2 / 2, qh2 * 0.1);
        drawRgbCircles(ctx, qw2, qh2);
        ctx.restore();

        // === Bottom-left: Masked blend ===
        ctx.save();
        ctx.translate(0, cy);
        // Draw striped columns as the content to be masked
        {
            let lastX = 0;
            let c1 = "#ff00ff", c2 = "#ffffff";
            for (let i = 0; i < kColumns; i++) {
                const x = Math.floor(qw3 * (i + 1) / kColumns);
                ctx.setBlendMode("alpha");
                ctx.fillStyle = c1;
                [c1, c2] = [c2, c1];
                ctx.fillRect(lastX, 0, x - lastX, qh3);
                lastX = x;
            }
        }
        // Clear region then let circle shapes through as mask
        ctx.setBlendMode("maskRemove");
        ctx.fillStyle = "#ffffff";
        ctx.fillRect(0, 0, qw3, qh3);
        ctx.setBlendMode("maskAdd");
        ctx.fillStyle = "#ffffff";
        drawRgbCircles(ctx, qw3, qh3);
        // Label drawn on top in normal alpha mode
        ctx.setBlendMode("alpha");
        ctx.fillStyle = "#ffffff";
        ctx.fillText("Masked", qw3 / 2, qh3 * 0.1);
        ctx.restore();

        // === Bottom-right: Grouped Transparency ===
        // beginLayer renders all children at full opacity into an isolated buffer,
        // then composites the entire buffer at 50% — matching Frame::setAlphaTransparency(0.5f).
        ctx.save();
        ctx.translate(cx, cy);
        ctx.beginLayer({ opacity: 0.5 });
        ctx.fillStyle = "#ffffff";
        ctx.fillText("Grouped Transparency", qw4 / 2, qh4 * 0.1);
        drawRgbCircles(ctx, qw4, qh4);
        ctx.endLayer();
        ctx.restore();
    }

    resized() {}
}

startUI(ctx, Editor);