import { getParameter as _getParameter, setParameter as _setParameter } from 'native:parameters';
import { addEventListener } from 'native:events';

const widgets = [];

export function _registerWidget(w) { widgets.push(w); }

addEventListener('mousedown', (x, y) => {
    for (const w of widgets)
        if (w.hitTest(x, y)) { w.onMouseDown(x, y); break; }
});

addEventListener('mousemove', (x, y) => {
    for (const w of widgets)
        if (w.hitTest(x, y)) { w.onMouseMove(x, y); break; }
});

addEventListener('mouseup', (x, y) => {
    for (const w of widgets)
        if (w.hitTest(x, y)) { w.onMouseUp(x, y); break; }
});

export class Widget {
    constructor(ctx, x, y, width, height) {
        this.ctx = ctx;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        _registerWidget(this);
    }

    getParameter(id)         { return _getParameter(id); }
    setParameter(id, value)  { _setParameter(id, value); }

    hitTest(x, y) {
        return x >= this.x && x <= this.x + this.width &&
               y >= this.y && y <= this.y + this.height;
    }

    onMouseDown(x, y) {}
    onMouseUp(x, y)   {}
    onMouseMove(x, y) {}
    draw()            {}
}