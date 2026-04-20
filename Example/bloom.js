import { startUI, Component } from "../widgets/ui.js";

const NUM_DOTS = 10;
const DOT_RADIUS = 5;
const NUM_WAVE_POINTS = 1200;
const PI = Math.PI;

const RAINBOW = [
    [0.00, "#ff6666"],
    [0.17, "#ffff66"],
    [0.33, "#66ff66"],
    [0.50, "#66ffff"],
    [0.67, "#6666ff"],
    [0.83, "#ff66ff"],
    [1.00, "#ff6666"],
];

function makeRainbowGradient(ctx, W) {
    const grad = ctx.createLinearGradient(0, 0, W, 0);
    for (const [stop, hex] of RAINBOW) {
        grad.addColorStop(stop, hex);
    }
    return grad;
}

// Returns 0..1 boost strength at a given screen X position
function localBoost(x, W, boostPhase) {
    const t = x / W;
    const dist = Math.abs(boostPhase - t);
    return Math.max(0, (0.13 - dist) / 0.13);
}

class BloomEditor extends Component {
    constructor() {
        super();
        this._bloomSet = false;
    }

    paint(ctx) {
        if (!this._bloomSet) {
            ctx.setBloom(30, 2.0);
            this._bloomSet = true;
        }

        const W = this.width;
        const H = this.height;
        const t = ctx.time();

        ctx.fillStyle = "#22282d";
        ctx.fillRect(0, 0, W, H);

        // boostPhase travels from -0.25 to 1.25 (off-screen left to off-screen right)
        const boostTime  = t * 0.2;
        const boostPhase = (boostTime - Math.floor(boostTime)) * 1.5 - 0.25;

        // Pure rainbow gradient — no boost/clamp so colors stay vivid
        const grad = makeRainbowGradient(ctx, W);

        // ── DOTS ──────────────────────────────────────────────────────────────
        const centerY = H * 0.125;

        ctx.fillStyle = grad;
        ctx.globalAlpha = 1.0;
        for (let i = 0; i < NUM_DOTS; i++) {
            const cx = ((i + 1) / (NUM_DOTS + 1.0)) * W;
            const b = localBoost(cx, W, boostPhase);
            // hdrMultiplier > 1 at hotspot → pixel exceeds bloom threshold
            ctx.hdrMultiplier = 1.0 + b * 0.4;
            ctx.circle(cx, centerY,     DOT_RADIUS);
            ctx.circle(cx, H - centerY, DOT_RADIUS);
        }
        ctx.hdrMultiplier = 1.0;

        // ── WAVE ──────────────────────────────────────────────────────────────
        let position = 0;
        const linePhase = t * 0.5;
        const px = [], py = [];
        for (let i = 0; i < NUM_WAVE_POINTS; i++) {
            const frac = 1.1 * i / (NUM_WAVE_POINTS - 1.0) - 0.05;
            const delta = Math.min(frac, 1.0 - frac);
            position += 0.02 * delta * delta + 0.003;
            px.push((i / (NUM_WAVE_POINTS - 1.0)) * W);
            py.push((0.5 + Math.sin((linePhase + position) * 2.0 * PI) * 0.25) * H);
        }

        // Draw at hdrMultiplier=1 everywhere — hotspot gets extra boost via per-segment draws
        ctx.strokeStyle = grad;
        ctx.lineWidth   = 2.5;
        ctx.lineCap     = "round";
        ctx.lineJoin    = "round";
        ctx.setLineDash([]);
        ctx.globalAlpha = 1.0;

        // Base wave at hdr=1 (no bloom)
        ctx.hdrMultiplier = 1.0;
        ctx.beginPath();
        for (let i = 0; i < NUM_WAVE_POINTS; i++) {
            if (i === 0) ctx.moveTo(px[i], py[i]);
            else         ctx.lineTo(px[i], py[i]);
        }
        ctx.stroke();

        // Hotspot segment at hdr > 1 (triggers bloom)
        ctx.hdrMultiplier = 1.3;
        ctx.beginPath();
        let boostStarted = false;
        for (let i = 0; i < NUM_WAVE_POINTS; i++) {
            const b = localBoost(px[i], W, boostPhase);
            if (b > 0) {
                if (!boostStarted) { ctx.moveTo(px[i], py[i]); boostStarted = true; }
                else ctx.lineTo(px[i], py[i]);
            } else if (boostStarted) { break; }
        }
        if (boostStarted) ctx.stroke();
        ctx.hdrMultiplier = 1.0;

        ctx.globalAlpha = 1.0;
    }
}

startUI(ctx, BloomEditor);
