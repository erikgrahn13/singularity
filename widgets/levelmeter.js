import { Component, getParameter } from "singularity";

// A read-only level meter driven by a normalized output parameter (0..1).
export function LevelMeter({ x = 0, y = 0, width = 24, height = 160,
                             parameterId, minDb = -60, peakHoldMs = 1000,
                             releaseDbPerSecond = 15, theme = {} }) {
  const t = {
    backgroundColor: "#0d1118",
    trackColor:      "#171d27",
    lowColor:        "color(srgb-linear 0.02 1.05 1.15 1.0)",
    midColor:        "color(srgb-linear 1.15 0.72 0.04 1.0)",
    highColor:       "color(srgb-linear 1.20 0.04 0.03 1.0)",
    peakColor:       "#e8edf5",
    borderColor:     "#2b3442",
    bloom:           0.18,
    cornerRadius:    4,
    innerRadius:     2,
    padding:         5,
    ...theme,
  };

  let displayed = 0;
  let held = 0;
  let holdUntil = 0;
  let previousTime = Date.now();
  let meterGradient = null;

  function normalizedLevel(linear) {
    if (!(linear > 0)) return 0;
    const db = 20 * Math.log10(linear);
    return Math.max(0, Math.min(1, (db - minDb) / -minDb));
  }

  return Component({
    x,
    y,
    width,
    height,
    animate: true,

    draw(ctx) {
      const now = Date.now();
      const elapsed = Math.max(0, now - previousTime);
      previousTime = now;

      const value = getParameter(parameterId);
      const target = Number.isNaN(value) ? 0 : normalizedLevel(value);
      const release = releaseDbPerSecond * (elapsed / 1000) / Math.max(1, -minDb);

      // Peak attack is instantaneous; release is linear in dB display space.
      if (target >= displayed) {
        displayed = target;
      } else {
        displayed = Math.max(target, displayed - release);
      }

      if (displayed >= held) {
        held = displayed;
        holdUntil = now + peakHoldMs;
      } else if (now > holdUntil) {
        held = Math.max(displayed, held - release);
      }

      const p = t.padding;
      const innerWidth = Math.max(0, width - p * 2);
      const innerHeight = Math.max(0, height - p * 2);
      const fillHeight = innerHeight * displayed;
      const fillY = p + innerHeight - fillHeight;

      if (!meterGradient) {
        // Gradient runs bottom-to-top and remains anchored to the full scale,
        // regardless of how many segments are currently active.
        meterGradient = ctx.createLinearGradient(0, p + innerHeight, 0, p);
        meterGradient.addColorStop(0.00, t.lowColor);
        meterGradient.addColorStop(0.78, t.lowColor);
        meterGradient.addColorStop(0.90, t.midColor);
        meterGradient.addColorStop(0.98, t.highColor);
        meterGradient.addColorStop(1.00, t.highColor);
      }

      ctx.save();
      ctx.bloom = t.bloom;
      ctx.fillStyle = t.backgroundColor;
      ctx.shadowBlur = 0;
      ctx.beginPath();
      ctx.roundRect(0, 0, width, height, t.cornerRadius);
      ctx.fill();

      // Recessed continuous track.
      ctx.fillStyle = t.trackColor;
      ctx.beginPath();
      ctx.roundRect(p, p, innerWidth, innerHeight, t.innerRadius);
      ctx.fill();

      // Continuous liquid-style level, using one gradient anchored to the
      // complete dB scale rather than changing with the current value.
      if (fillHeight > 0) {
        const activeRadius = Math.min(t.innerRadius, fillHeight / 2, innerWidth / 2);
        ctx.fillStyle = meterGradient;
        ctx.beginPath();
        ctx.roundRect(p, fillY, innerWidth, fillHeight, activeRadius);
        ctx.fill();

      }
      ctx.restore();

      ctx.save();
      if (held > 0) {
        const peakY = p + (1 - held) * innerHeight;
        ctx.fillStyle = t.peakColor;
        ctx.shadowBlur = 0;
        ctx.fillRect(p + 1, peakY, Math.max(0, innerWidth - 2), 1);
      }
      ctx.shadowBlur = 0;
      ctx.strokeStyle = t.borderColor;
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.roundRect(0, 0, width, height, t.cornerRadius);
      ctx.stroke();
      ctx.restore();
    },
  });
}
