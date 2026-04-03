import { RootWidget } from "./root.js";
import { Widget } from "./widget.js";

let root = null;
let content = null;

export class Component extends Widget {
    constructor() {
        super();
    }
}

export function startUI(ctx, ContentComponentClass) {
    root = new RootWidget(ctx);

    content = new ContentComponentClass();
    content.setBounds(0, 0, ctx.canvas.width, ctx.canvas.height);

    root.addChild(content);
    root.requestRepaint();
}

export function getContentComponent() {
    return content;
}

export function renderUI() {
    if (!root) return;
    root.flushRepaint();
}

globalThis.renderUI = renderUI;