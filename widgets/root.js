import { Widget } from "./widget.js";
import { Button } from "./button.js";
import { addEventListener } from "native:events";

export class RootWidget extends Widget {
    constructor(ctx) {
        super(0, 0, ctx.canvas.width, ctx.canvas.height);

        this.ctx = ctx;
        this.repaintPending = true;
        this.mouseCapture = null;

        addEventListener("mousedown", (x, y) => this.handleMouseDown(x, y));
        addEventListener("mousemove", (x, y) => this.handleMouseMove(x, y));
        addEventListener("mouseup", (x, y) => this.handleMouseUp(x, y));
    }

    addStandaloneOverlays() {
        if (typeof STANDALONE !== 'undefined' && STANDALONE) {
            this.settingsButton = new Button(0, 0, 120, 60, () => openSettingsWindow(), { label: "Settings" });
            this.addChild(this.settingsButton);
        }
    }

    requestRepaint() {
        // console.log("request paint");
        this.repaintPending = true;
    }

    flushRepaint() {
        if (!this.repaintPending) return;
        this.repaintPending = false;
        this.draw();
    }

    draw() {
        const ctx = this.ctx;

        // For now use a solid background fill instead of clearRect
        ctx.fillStyle = "#000000";
        ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);

        for (const child of this.children) {
            this.paintSubtree(ctx, child);
        }

        for (const child of this.children) {
            this.paintOverlays(ctx, child);
        }
    }

    paintSubtree(ctx, widget) {
        if (!widget.visible) return;

        ctx.save();
        ctx.translate(widget.x, widget.y);

        widget.paint(ctx);

        for (const child of widget.children) {
            this.paintSubtree(ctx, child);
        }

        ctx.restore();
    }

    paintOverlays(ctx, widget) {
        if (!widget.visible) return;

        ctx.save();
        ctx.translate(widget.x, widget.y);

        for (const child of widget.children) {
            this.paintOverlays(ctx, child);
        }

        widget.paintOverChildren(ctx);
        ctx.restore();
    }

    findWidgetAt(x, y, widget = this) {
        if (!widget.visible) return null;
        if (!widget.hitTest(x, y)) return null;

        // First pass: overlay widgets (e.g. open dropdowns) take priority
        for (let i = widget.children.length - 1; i >= 0; --i) {
            const child = widget.children[i];
            if (!child.isShowingOverlay?.()) continue;
            const hit = this.findWidgetAt(x - child.x, y - child.y, child);
            if (hit) return hit;
        }

        // Second pass: normal widgets
        for (let i = widget.children.length - 1; i >= 0; --i) {
            const child = widget.children[i];
            if (child.isShowingOverlay?.()) continue;
            const hit = this.findWidgetAt(x - child.x, y - child.y, child);
            if (hit) return hit;
        }

        return widget;
    }

    handleMouseDown(x, y) {
        const target = this.findWidgetAt(x, y);
        if (!target || target === this) return;

        this.mouseCapture = target;
        const p = target.globalToLocal(x, y);
        target.onMouseDown(p.x, p.y);
    }

    handleMouseMove(x, y) {
        const target = this.mouseCapture || this.findWidgetAt(x, y);
        if (!target || target === this) return;

        const p = target.globalToLocal(x, y);
        target.onMouseMove(p.x, p.y);
    }

    handleMouseUp(x, y) {
        const target = this.mouseCapture || this.findWidgetAt(x, y);

        if (!target || target === this) {
            this.mouseCapture = null;
            return;
        }

        const p = target.globalToLocal(x, y);
        target.onMouseUp(p.x, p.y);
        this.mouseCapture = null;
    }
}