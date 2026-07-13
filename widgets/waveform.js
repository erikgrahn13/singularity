import { Component, getAudioData } from "singularity";

// A scrolling time-domain waveform. The view keeps enough continuous audio to
// move at a readable pace and applies display-only gain so quieter material
// still uses the available height.
export function Waveform({
  x = 0,
  y = 0,
  width = 320,
  height = 120,
  color = "#70f6ff",
  fillColor = "rgba(112,246,255,0.18)",
  backgroundColor = "#12091f",
  gridColor = "rgba(255,255,255,0.08)",
  windowMs = 1500,
  historyFrames = null,
  gain = 2,
  autoGain = false,
  maxGain = 12,
} = {}) {
  const history = [];
  let historyStart = 0;
  let historyOffset = 0;
  let lastRevision = 0;
  let sampleRate = 48000;
  let displayGain = Math.max(0, gain);
  let gainInitialized = false;
  let previousTime = Date.now();
  const positions = [];
  const upper = [];
  const lower = [];

  function appendAudioData(audioData) {
    if (!audioData || audioData.revision === lastRevision)
      return;

    lastRevision = audioData.revision;
    sampleRate = audioData.sampleRate > 0 ? audioData.sampleRate : sampleRate;

    const interleaved = audioData.samples ?? [];
    const channels = Math.max(1, audioData.numChannels ?? 1);
    const frames = Math.floor(interleaved.length / channels);

    for (let frame = 0; frame < frames; ++frame) {
      // Keep the strongest channel rather than averaging channels together.
      // Averaging makes out-of-phase stereo material appear nearly silent.
      let sample = 0;
      for (let channel = 0; channel < channels; ++channel) {
        const candidate = interleaved[frame * channels + channel] ?? 0;
        if (Math.abs(candidate) > Math.abs(sample))
          sample = candidate;
      }
      history.push(Math.max(-1, Math.min(1, sample)));
    }

    const visibleFrames = Math.max(2, Math.round(sampleRate * windowMs / 1000));
    const capacity = Math.max(2, historyFrames ?? visibleFrames * 2);
    const activeFrames = history.length - historyStart;
    if (activeFrames > capacity)
      historyStart += activeFrames - capacity;

    // Move the live portion back to index zero occasionally instead of
    // shifting a large history array on every animation frame.
    if (historyStart > capacity) {
      history.splice(0, historyStart);
      historyOffset += historyStart;
      historyStart = 0;
    }
  }

  function updateDisplayGain(start, end) {
    if (!autoGain) {
      displayGain = Math.max(0, gain);
      return;
    }

    let peak = 0;
    for (let i = start; i < end; ++i)
      peak = Math.max(peak, Math.abs(history[i]));

    const target = autoGain && peak > 0.0001
      ? Math.max(1, Math.min(maxGain, 0.82 / peak))
      : 1;

    if (!gainInitialized) {
      displayGain = target;
      gainInitialized = true;
      previousTime = Date.now();
      return;
    }

    const now = Date.now();
    const elapsed = Math.min(100, Math.max(0, now - previousTime));
    previousTime = now;

    // Pull gain down quickly on loud transients, but raise it gently to avoid
    // visible pumping during quieter passages.
    const responseMs = target < displayGain ? 35 : 180;
    const amount = 1 - Math.exp(-elapsed / responseMs);
    displayGain += (target - displayGain) * amount;
  }

  return Component({
    x, y, width, height, animate: true,
    draw(ctx) {
      appendAudioData(getAudioData());

      ctx.save();
      ctx.fillStyle = backgroundColor;
      ctx.fillRect(0, 0, width, height);

      // A restrained scope grid gives the motion a stable visual reference.
      ctx.strokeStyle = gridColor;
      ctx.lineWidth = 1;
      for (let division = 1; division < 4; ++division) {
        const gridX = Math.round(width * division / 4) + 0.5;
        const gridY = Math.round(height * division / 4) + 0.5;
        ctx.beginPath();
        ctx.moveTo(gridX, 0);
        ctx.lineTo(gridX, height);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(0, gridY);
        ctx.lineTo(width, gridY);
        ctx.stroke();
      }

      const availableFrames = history.length - historyStart;
      if (availableFrames < 2) {
        ctx.restore();
        return;
      }

      const requestedFrames = Math.max(2, Math.round(sampleRate * windowMs / 1000));
      const latestFrame = historyOffset + history.length;
      const firstAvailableFrame = historyOffset + historyStart;
      const displayStartFrame = latestFrame - requestedFrames;
      const visibleStartFrame = Math.max(firstAvailableFrame, displayStartFrame);
      const start = visibleStartFrame - historyOffset;
      const end = history.length;
      updateDisplayGain(start, end);

      const columns = Math.max(1, Math.floor(width));
      const framesPerColumn = Math.max(1, Math.ceil(requestedFrames / columns));
      const firstBucket = Math.floor(visibleStartFrame / framesPerColumn) * framesPerColumn;
      const centerY = height * 0.5;
      const amplitude = Math.max(1, height * 0.44);
      let pointCount = 0;

      // Peak buckets are anchored to absolute sample positions. Once a bucket
      // is complete its height never changes; it only moves horizontally.
      for (let bucket = firstBucket; bucket < latestFrame; bucket += framesPerColumn) {
        const first = Math.max(firstAvailableFrame, bucket) - historyOffset;
        const last = Math.min(latestFrame, bucket + framesPerColumn) - historyOffset;
        let minimum = 1;
        let maximum = -1;

        for (let i = first; i < last; ++i) {
          minimum = Math.min(minimum, history[i]);
          maximum = Math.max(maximum, history[i]);
        }

        positions[pointCount] = (bucket - displayStartFrame) * width / requestedFrames;
        upper[pointCount] = centerY - Math.max(-1, Math.min(1, maximum * displayGain)) * amplitude;
        lower[pointCount] = centerY - Math.max(-1, Math.min(1, minimum * displayGain)) * amplitude;
        ++pointCount;
      }
      positions.length = pointCount;
      upper.length = pointCount;
      lower.length = pointCount;

      // Fill the min/max envelope, then trace both edges. Each horizontal pixel
      // represents a slice of real time, so transients remain visible while the
      // waveform scrolls naturally.
      ctx.beginPath();
      ctx.moveTo(positions[0], upper[0]);
      for (let point = 1; point < pointCount; ++point)
        ctx.lineTo(positions[point], upper[point]);
      for (let point = pointCount - 1; point >= 0; --point)
        ctx.lineTo(positions[point], lower[point]);
      ctx.closePath();
      ctx.fillStyle = fillColor;
      ctx.fill();

      ctx.strokeStyle = color;
      ctx.lineWidth = 1.5;
      ctx.lineCap = "round";
      ctx.beginPath();
      ctx.moveTo(positions[0], upper[0]);
      for (let point = 1; point < pointCount; ++point)
        ctx.lineTo(positions[point], upper[point]);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(positions[0], lower[0]);
      for (let point = 1; point < pointCount; ++point)
        ctx.lineTo(positions[point], lower[point]);
      ctx.stroke();
      ctx.restore();
    },
  });
}
