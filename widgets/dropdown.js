import { Widget } from "./widget.js";

export class Dropdown extends Widget {
    constructor(x = 0, y = 0, width = 200, height = 30, items = [], onChange = null, theme = {}) {
        super(x, y, width, height);
        this.items = items;
        this.selectedIndex = 0;
        this.open = false;
        this.onChange = onChange;

        this.theme = {
            background:       "#1e1e2e",
            border:           "#3a3a5c",
            hoverBackground:  "#2a2a3e",
            activeBackground: "#00d4ff",
            activeColor:      "#000000",
            textColor:        "#ffffff",
            fontSize:         12,
            itemHeight:       28,
            ...theme
        };

        this._hoveredIndex = -1;
    }

    get selectedItem() {
        return this.items[this.selectedIndex] ?? null;
    }

    setItems(newItems) {
        this.items = newItems;
        this.selectedIndex = 0;
        this.open = false;
        this._hoveredIndex = -1;
        this.repaint();
    }

    onMouseDown(x, y) {
        if (!this.open) {
            this.open = true;
            this.repaint();
            return;
        }

        // Click on the header while open — close
        if (y < this.height) {
            this.open = false;
            this.repaint();
            return;
        }

        // Click inside the list
        const listY = y - this.height;
        const idx = Math.floor(listY / this.theme.itemHeight);
        if (idx >= 0 && idx < this.items.length) {
            this.selectedIndex = idx;
            if (this.onChange) this.onChange(idx, this.items[idx]);
        }
        this.open = false;
        this.repaint();
    }

    onMouseMove(x, y) {
        if (!this.open) return;
        const listY = y - this.height;
        const idx = Math.floor(listY / this.theme.itemHeight);
        const hovered = (idx >= 0 && idx < this.items.length) ? idx : -1;
        if (hovered !== this._hoveredIndex) {
            this._hoveredIndex = hovered;
            this.repaint();
        }
    }

    onMouseUp(x, y) {}

    isShowingOverlay() { return this.open; }

    // Override hitTest to include the open list area
    hitTest(x, y) {
        if (!this.open) return x >= 0 && y >= 0 && x < this.width && y < this.height;
        const listHeight = this.items.length * this.theme.itemHeight;
        return x >= 0 && y >= 0 && x < this.width && y < this.height + listHeight;
    }

    paint(ctx) {
        const t = this.theme;

        // Header background
        ctx.fillStyle = t.background;
        ctx.fillRect(0, 0, this.width, this.height);

        // Header border
        ctx.strokeStyle = this.open ? t.activeBackground : t.border;
        ctx.lineWidth = 1;
        ctx.strokeRect(0, 0, this.width, this.height);

        // Selected label
        ctx.font = `${t.fontSize}px sans-serif`;
        ctx.textAlign = "left";
        ctx.textBaseline = "middle";
        ctx.fillStyle = t.textColor;
        ctx.fillText(this.items[this.selectedIndex] ?? "", 8, this.height / 2);

        // Chevron
        const cx = this.width - 14;
        const cy = this.height / 2;
        const a = this.open ? -1 : 1;
        ctx.beginPath();
        ctx.moveTo(cx - 5, cy - 3 * a);
        ctx.lineTo(cx,     cy + 3 * a);
        ctx.lineTo(cx + 5, cy - 3 * a);
        ctx.strokeStyle = t.textColor;
        ctx.lineWidth = 1.5;
        ctx.stroke();
    }

    paintOverChildren(ctx) {
        if (!this.open) return;

        const t = this.theme;
        const iH = t.itemHeight;

        for (let i = 0; i < this.items.length; i++) {
            const iy = this.height + i * iH;
            const isSelected = i === this.selectedIndex;
            const isHovered  = i === this._hoveredIndex;

            ctx.fillStyle = isSelected ? t.activeBackground
                           : isHovered  ? t.hoverBackground
                           : t.background;
            ctx.fillRect(0, iy, this.width, iH);

            ctx.strokeStyle = t.border;
            ctx.lineWidth = 1;
            ctx.strokeRect(0, iy, this.width, iH);

            ctx.font = `${t.fontSize}px sans-serif`;
            ctx.textAlign = "left";
            ctx.textBaseline = "middle";
            ctx.fillStyle = isSelected ? t.activeColor : t.textColor;
            ctx.fillText(this.items[i], 8, iy + iH / 2);
        }
    }
}
