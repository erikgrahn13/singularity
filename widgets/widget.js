export class Widget {
    constructor(x = 0, y = 0, width = 0, height = 0) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;

        this.parent = null;
        this.children = [];
        this.visible = true;
    }

    addChild(child) {
        if (child.parent) {
            child.parent.removeChild(child);
        }

        child.parent = this;
        this.children.push(child);

        this.resized();
        this.repaint();
    }

    removeChild(child) {
        const i = this.children.indexOf(child);
        if (i !== -1) {
            this.children.splice(i, 1);
            child.parent = null;
            this.resized();
            this.repaint();
        }
    }

    setBounds(x, y, width, height) {
        const changed =
            this.x !== x ||
            this.y !== y ||
            this.width !== width ||
            this.height !== height;

        if (!changed) return;

        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;

        this.resized();
        this.repaint();
    }

    repaint() {
        const root = this.getRoot();
        if (root && typeof root.requestRepaint === "function") {
            root.requestRepaint();
        }
    }

    getRoot() {
        let node = this;
        while (node.parent) {
            node = node.parent;
        }
        return node;
    }

    getAbsolutePosition() {
        let x = this.x;
        let y = this.y;
        let p = this.parent;

        while (p) {
            x += p.x;
            y += p.y;
            p = p.parent;
        }

        return { x, y };
    }

    globalToLocal(x, y) {
        const abs = this.getAbsolutePosition();
        return {
            x: x - abs.x,
            y: y - abs.y
        };
    }

    hitTest(x, y) {
        return x >= 0 && y >= 0 && x < this.width && y < this.height;
    }

    resized() {}
    paint(ctx) {}
    paintOverChildren(ctx) {}

    onMouseDown(x, y) {}
    onMouseMove(x, y) {}
    onMouseUp(x, y) {}
}