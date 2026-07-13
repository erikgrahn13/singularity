import { Component, getAudioData } from "singularity";

export function Waveform({ x = 0, y = 0, width = 320, height = 120, color = "#70f6ff", backgroundColor = "#12091f" } = {}) {
  return Component({
    x, y, width, height, animate: true, backgroundColor,
    draw(ctx) {
      const audioData = getAudioData();
      const interleaved = audioData?.samples ?? [];
      const channels = Math.max(1, audioData?.numChannels ?? 1);
      const frames = Math.floor(interleaved.length / channels);

      ctx.fillStyle = backgroundColor;
      ctx.fillRect(0, 0, width, height);
      ctx.strokeStyle = "rgba(255,255,255,0.22)";
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, height * 0.5);
      ctx.lineTo(width, height * 0.5);
      ctx.stroke();

      if (!frames) return;

      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      ctx.beginPath();
      for (let px = 0; px < width; ++px) {
        const start = Math.floor((px / width) * frames);
        const end = Math.max(start + 1, Math.floor(((px + 1) / width) * frames));
        let min = 1;
        let max = -1;
        for (let frame = start; frame < end && frame < frames; ++frame) {
          let sum = 0;
          for (let channel = 0; channel < channels; ++channel)
            sum += interleaved[frame * channels + channel] ?? 0;
          const sample = Math.max(-1, Math.min(1, sum / channels));
          min = Math.min(min, sample);
          max = Math.max(max, sample);
        }
        const yMin = (1 - max) * 0.5 * height;
        const yMax = (1 - min) * 0.5 * height;
        if (px === 0) ctx.moveTo(px, yMin);
        ctx.lineTo(px, yMin);
        ctx.lineTo(px, yMax);
      }
      ctx.stroke();
    },
  });
}
