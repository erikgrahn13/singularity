import { RootWidget } from "./root.js";
import { Widget } from "./widget.js";

let root = null;
let content = null;

export class Component extends Widget {
    constructor() {
        super();
    }
}

export function startUI(ctx, ContentComponentClass, { showOverlays = true } = {}) {
    root = new RootWidget(ctx);

    content = new ContentComponentClass();
    content.setBounds(0, 0, ctx.canvas.width, ctx.canvas.height);

    root.addChild(content);
    if (showOverlays) root.addStandaloneOverlays(); // add settings button on top of editor
    root.requestRepaint();
}

export function getContentComponent() {
    return content;
}

export function renderUI() {
    if (!root) return;
    root.draw();
}

globalThis.renderUI = renderUI;