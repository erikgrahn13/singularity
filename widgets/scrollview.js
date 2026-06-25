import { Component } from "singularity";

export function ScrollView({
  x = 0,
  y = 0,
  width = 260,
  height = 220,
  items = [],
  itemHeight = 26,
  onItemClick = null,
  theme = {},
}) {
  const t = {
    background: "#11131a",
    border: "#3d4257",
    textColor: "#e9ecf7",
    itemHoverBackground: "#1d2333",
    itemActiveBackground: "#26314b",
    itemSelectedBackground: "#34508f",
    itemSelectedTextColor: "#ffffff",
    scrollbarTrack: "#0d1018",
    scrollbarThumb: "#5a678d",
    padding: 8,
    fontSize: 14,
    cornerRadius: 8,
    textYOffset: 1,
    ...theme,
  };

  let scrollOffset = 0;
  let hoveredIndex = -1;
  let pressedIndex = -1;
  let selectedIndex = -1;
  let draggingThumb = false;
  let thumbDragOffsetY = 0;

  const contentHeight = () => items.length * itemHeight;
  const viewportHeight = () => Math.max(0, height - t.padding * 2);
  const maxScroll = () => Math.max(0, contentHeight() - viewportHeight());
  const contentStartOffset = () => {
    const spare = viewportHeight() - contentHeight();
    return spare > 0 ? spare * 0.5 : 0;
  };

  function clampScroll(v) {
    const m = maxScroll();
    if (v < 0) return 0;
    if (v > m) return m;
    return v;
  }

  function itemIndexAt(localX, localY) {
    const innerLeft = t.padding;
    const innerTop = t.padding;
    const innerRight = width - t.padding;
    const innerBottom = height - t.padding;

    if (
      localX < innerLeft ||
      localX > innerRight ||
      localY < innerTop ||
      localY > innerBottom
    ) {
      return -1;
    }

    const yInContent = localY - innerTop - contentStartOffset() + scrollOffset;
    const idx = Math.floor(yInContent / itemHeight);
    return idx >= 0 && idx < items.length ? idx : -1;
  }

  function getScrollbarMetrics() {
    const innerH = viewportHeight();
    const m = maxScroll();
    if (m <= 0) return null;

    const trackW = 8;
    const trackX = width - t.padding - trackW;
    const trackY = t.padding;
    const trackH = innerH;
    const thumbH = Math.max(24, (innerH / contentHeight()) * trackH);
    const thumbY = trackY + (scrollOffset / m) * (trackH - thumbH);

    return { trackW, trackX, trackY, trackH, thumbH, thumbY };
  }

  return Component({
    x,
    y,
    width,
    height,

    onMouseDown: (e) => {
      const sb = getScrollbarMetrics();
      if (sb) {
        const hitTrack =
          e.x >= sb.trackX &&
          e.x <= sb.trackX + sb.trackW &&
          e.y >= sb.trackY &&
          e.y <= sb.trackY + sb.trackH;

        const hitThumb =
          hitTrack &&
          e.y >= sb.thumbY &&
          e.y <= sb.thumbY + sb.thumbH;

        if (hitThumb) {
          draggingThumb = true;
          thumbDragOffsetY = e.y - sb.thumbY;
          pressedIndex = -1;
          return;
        }

        if (hitTrack) {
          const thumbRange = sb.trackH - sb.thumbH;
          if (thumbRange > 0) {
            const targetThumbY = Math.max(
              sb.trackY,
              Math.min(sb.trackY + thumbRange, e.y - sb.thumbH * 0.5)
            );
            const ratio = (targetThumbY - sb.trackY) / thumbRange;
            scrollOffset = clampScroll(ratio * maxScroll());
          }
          draggingThumb = true;
          thumbDragOffsetY = sb.thumbH * 0.5;
          pressedIndex = -1;
          return;
        }
      }

      pressedIndex = itemIndexAt(e.x, e.y);
      hoveredIndex = pressedIndex;
    },

    onMouseUp: (e) => {
      if (draggingThumb) {
        draggingThumb = false;
        pressedIndex = -1;
        return;
      }

      const releaseIndex = itemIndexAt(e.x, e.y);
      if (pressedIndex !== -1 && releaseIndex === pressedIndex) {
        selectedIndex = releaseIndex;
        if (typeof onItemClick === "function") {
          onItemClick(releaseIndex, items[releaseIndex]);
        }
      }
      pressedIndex = -1;
    },

    onMouseDrag: (e) => {
      if (draggingThumb) {
        const sb = getScrollbarMetrics();
        if (!sb) {
          draggingThumb = false;
          return;
        }

        const thumbRange = sb.trackH - sb.thumbH;
        if (thumbRange <= 0) return;

        const thumbTop = Math.max(
          sb.trackY,
          Math.min(sb.trackY + thumbRange, e.y - thumbDragOffsetY)
        );
        const ratio = (thumbTop - sb.trackY) / thumbRange;
        scrollOffset = clampScroll(ratio * maxScroll());
        return;
      }

      hoveredIndex = itemIndexAt(e.x, e.y);
    },

    onMouseMove: (e) => {
      hoveredIndex = itemIndexAt(e.x, e.y);
    },

    onMouseExit: () => {
      if (!draggingThumb) {
        hoveredIndex = -1;
      }
    },

    onMouseWheel: (e) => {
      scrollOffset = clampScroll(scrollOffset - e.deltaY * 18);
    },

    draw: (ctx) => {
      const r = t.cornerRadius;
      const innerX = t.padding;
      const innerY = t.padding;
      const innerW = width - t.padding * 2;
      const innerH = viewportHeight();
      const contentOffsetY = contentStartOffset();

      ctx.save();

      ctx.fillStyle = t.background;
      ctx.beginPath();
      ctx.roundRect(0, 0, width, height, r);
      ctx.fill();

      ctx.strokeStyle = t.border;
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.roundRect(0, 0, width, height, r);
      ctx.stroke();

      const startIndex = Math.max(0, Math.floor(scrollOffset / itemHeight));
      const endIndex = Math.min(
        items.length,
        Math.ceil((scrollOffset + innerH) / itemHeight) + 1
      );

      for (let i = startIndex; i < endIndex; i++) {
        const y = innerY + contentOffsetY + i * itemHeight - scrollOffset;
        const isHovered = i === hoveredIndex;
        const isPressed = i === pressedIndex;
        const isSelected = i === selectedIndex;

        if (isSelected || isHovered || isPressed) {
          ctx.fillStyle = isSelected
            ? t.itemSelectedBackground
            : isPressed
              ? t.itemActiveBackground
              : t.itemHoverBackground;
          ctx.fillRect(innerX, y, innerW - 10, itemHeight);
        }

        ctx.fillStyle = isSelected ? t.itemSelectedTextColor : t.textColor;
        ctx.font = `${t.fontSize}px sans-serif`;
        ctx.textAlign = "left";
        ctx.textBaseline = "alphabetic";
        const label = items[i] == null ? "" : String(items[i]);
        const metrics = typeof ctx.measureText === "function"
          ? ctx.measureText("Mg")
          : null;
        const ascent = metrics?.actualBoundingBoxAscent ?? t.fontSize * 0.8;
        const descent = metrics?.actualBoundingBoxDescent ?? t.fontSize * 0.2;
        const textHeight = ascent + descent;
        const textY = y + Math.max(0, (itemHeight - textHeight) * 0.5) + ascent + t.textYOffset;
        ctx.fillText(label, innerX + 8, textY);
      }

      const sb = getScrollbarMetrics();
      if (sb) {
        const thumbColor = draggingThumb ? "#7f8fbe" : t.scrollbarThumb;
        const sbRadius = Math.max(1, Math.min(sb.trackW * 0.5, 8));

        ctx.fillStyle = t.scrollbarTrack;
        ctx.beginPath();
        ctx.roundRect(sb.trackX, sb.trackY, sb.trackW, sb.trackH, sbRadius);
        ctx.fill();

        ctx.fillStyle = thumbColor;
        ctx.beginPath();
        ctx.roundRect(sb.trackX, sb.thumbY, sb.trackW, sb.thumbH, sbRadius);
        ctx.fill();
      }

      ctx.restore();
    },
  });
}