import { startUI, Component } from "../widgets/ui.js";
import { Widget } from "../widgets/widget.js";

const DRAG_RADIUS = 20;
const DOT_RADIUS = 5;

const GRADIENTS = [
    { name: "Two Color",  stops: [[0, "#ffff00"], [1, "#00aaff"]] },
    { name: "Rainbow",    stops: [[0, "#ff0000"], [0.167, "#ffff00"], [0.333, "#00ff00"],
                                  [0.5, "#00ffff"], [0.667, "#0000ff"], [0.833, "#ff00ff"], [1, "#ff0000"]] },
    { name: "Warm",       stops: [[0, "#ff0000"], [0.5, "#ffaa00"], [1, "#ffff00"]] },
    { name: "Cool",       stops: [[0, "#0000ff"], [0.5, "#00aaff"], [1, "#00ffff"]] },
];

class GradientPanel extends Widget {
    constructor(x, y, w, h, type, label) {
        super(x, y, w, h);
        this.type = type;   // 'linear' | 'radial'
        this.label = label;
        this.gradientStops = GRADIENTS[0].stops;
        this.p1 = { x: w * 0.33, y: h * 0.33 };
        this.p2 = { x: w * 0.66, y: h * 0.66 };
        this.activePoint = null;
        this.dragging = false;
    }

    setGradientStops(stops) {
        this.gradientStops = stops;
        this.repaint();
    }

    _buildGradient(ctx) {
        let g;
        if (this.type === 'linear') {
            g = ctx.createLinearGradient(this.p1.x, this.p1.y, this.p2.x, this.p2.y);
        } else {
            const r = Math.hypot(this.p2.x - this.p1.x, this.p2.y - this.p1.y);
            g = ctx.createRadialGradient(this.p1.x, this.p1.y, 0, this.p1.x, this.p1.y, r);
        }
        for (const [stop, color] of this.gradientStops) {
            g.addColorStop(stop, color);
        }
        return g;
    }

    _hitTest(lx, ly) {
        const d1sq = (lx - this.p1.x) ** 2 + (ly - this.p1.y) ** 2;
        const d2sq = (lx - this.p2.x) ** 2 + (ly - this.p2.y) ** 2;
        const rr = DRAG_RADIUS * DRAG_RADIUS;
        if (d1sq < rr && d1sq <= d2sq) return 'p1';
        if (d2sq < rr) return 'p2';
        return null;
    }

    paint(ctx) {
        // Rounded rect filled with gradient
        ctx.fillStyle = this._buildGradient(ctx);
        ctx.beginPath();
        ctx.roundRect(0, 0, this.width, this.height, 18);
        ctx.fill();

        // Label
        ctx.fillStyle = "#000000";
        ctx.font = "20px sans-serif";
        ctx.textAlign = "center";
        ctx.textBaseline = "top";
        ctx.fillText(this.label, this.width / 2, 8);

        // Hover / drag highlight circle
        if (this.activePoint) {
            const p = this.activePoint === 'p1' ? this.p1 : this.p2;
            ctx.fillStyle = this.dragging ? "rgba(255,255,255,0.67)" : "rgba(255,255,255,0.4)";
            ctx.beginPath();
            ctx.arc(p.x, p.y, DRAG_RADIUS, 0, Math.PI * 2);
            ctx.fill();
        }

        // Control point dots
        ctx.fillStyle = "#000000";
        ctx.beginPath();
        ctx.arc(this.p1.x, this.p1.y, DOT_RADIUS, 0, Math.PI * 2);
        ctx.fill();
        ctx.beginPath();
        ctx.arc(this.p2.x, this.p2.y, DOT_RADIUS, 0, Math.PI * 2);
        ctx.fill();
    }

    onMouseMove(x, y) {
        const prev = this.activePoint;
        this.activePoint = this.dragging ? this.activePoint : this._hitTest(x, y);

        if (this.dragging && this.activePoint) {
            const cx = Math.max(0, Math.min(this.width,  x));
            const cy = Math.max(0, Math.min(this.height, y));
            if (this.activePoint === 'p1') { this.p1.x = cx; this.p1.y = cy; }
            else                           { this.p2.x = cx; this.p2.y = cy; }
        }

        if (this.activePoint !== prev || this.dragging) this.repaint();
    }

    onMouseDown(x, y) {
        const hit = this._hitTest(x, y);
        if (hit) {
            this.activePoint = hit;
            this.dragging = true;
            this.repaint();
        }
    }

    onMouseUp(x, y) {
        if (this.dragging) {
            this.dragging = false;
            this.repaint();
        }
    }
}

class GradientButton extends Widget {
    constructor(x, y, w, h, label, onClick) {
        super(x, y, w, h);
        this.label = label;
        this.onClick = onClick;
        this.hovered = false;
    }

    paint(ctx) {
        ctx.fillStyle = this.hovered ? "#555555" : "#333333";
        ctx.beginPath();
        ctx.roundRect(0, 0, this.width, this.height, 8);
        ctx.fill();

        ctx.fillStyle = "#ffffff";
        ctx.font = "16px sans-serif";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText(this.label, this.width / 2, this.height / 2);
    }

    onMouseMove(x, y) {
        const inside = x >= 0 && y >= 0 && x < this.width && y < this.height;
        if (inside !== this.hovered) { this.hovered = inside; this.repaint(); }
    }

    onMouseDown(x, y) {
        this.onClick();
    }

    onMouseUp(x, y) {}
}

class Editor extends Component {
    constructor() {
        super();
        this.gradientIndex = 0;
        this.linearPanel = null;
        this.radialPanel = null;
        this.patternBtn = null;
        this.gradientBtn = null;
    }

    resized() {
        const W = this.width, H = this.height;
        const pad = 8;
        const btnH = Math.round(H * 0.12);
        const panelH = H - btnH - pad * 3;
        const panelW = (W - pad * 3) / 2;

        if (!this.linearPanel) {
            this.linearPanel = new GradientPanel(pad, pad, panelW, panelH, 'linear', 'Linear Gradient');
            this.radialPanel  = new GradientPanel(pad * 2 + panelW, pad, panelW, panelH, 'radial', 'Radial Gradient');

            this.gradientBtn = new GradientButton(
                pad, panelH + pad * 2, (W - pad * 2), btnH,
                "Gradient: " + GRADIENTS[0].name,
                () => this._cycleGradient()
            );

            this.addChild(this.linearPanel);
            this.addChild(this.radialPanel);
            this.addChild(this.gradientBtn);
        } else {
            this.linearPanel.setBounds(pad, pad, panelW, panelH);
            this.radialPanel.setBounds(pad * 2 + panelW, pad, panelW, panelH);
            this.gradientBtn.setBounds(pad, panelH + pad * 2, W - pad * 2, btnH);
        }
    }

    _cycleGradient() {
        this.gradientIndex = (this.gradientIndex + 1) % GRADIENTS.length;
        const g = GRADIENTS[this.gradientIndex];
        this.linearPanel.setGradientStops(g.stops);
        this.radialPanel.setGradientStops(g.stops);
        this.gradientBtn.label = "Gradient: " + g.name;
        this.repaint();
    }

    paint(ctx) {
        ctx.fillStyle = "#222222";
        ctx.fillRect(0, 0, this.width, this.height);
    }
}

startUI(ctx, Editor);