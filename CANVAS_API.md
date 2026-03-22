# Canvas API Implementation Tracker

## Drawing State
- [x] `save()`
- [x] `restore()`
- [x] `globalAlpha` (property)

## Styles & Colors
- [x] `fillStyle` (property)
- [x] `strokeStyle` (property)
- [x] `lineWidth` (property)
- [X] `lineCap` (property) — `'butt'`, `'round'`, `'square'`
- [x] `lineJoin` (property) — `'miter'`, `'round'`, `'bevel'`

## Rectangles
- [x] `fillRect(x, y, w, h)`
- [x] `strokeRect(x, y, w, h)`
- [ ] `clearRect(x, y, w, h)`
- [x] `roundRect(x, y, w, h, radii)` *(nice to have)*

## Paths
- [x] `beginPath()`
- [x] `arc(x, y, r, startAngle, endAngle, ccw?)` — missing 6th `ccw` boolean
- [x] `fill()`
- [x] `stroke()`
- [x] `moveTo(x, y)`
- [x] `lineTo(x, y)`
- [x] `closePath()`
- [x] `quadraticCurveTo(cpx, cpy, x, y)`
- [x] `bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y)`
- [x] `arcTo(x1, y1, x2, y2, r)`
- [x] `ellipse(x, y, rx, ry, rotation, startAngle, endAngle, ccw?)`
- [x] `rect(x, y, w, h)`

## Text
- [x] `fillText(text, x, y)`
- [x] `strokeText(text, x, y)`
- [x] `measureText(text)` — returns `{ width }`
- [x] `font` (property)
- [x] `textAlign` (property) — `'left'`, `'center'`, `'right'`
- [x] `textBaseline` (property) — `'top'`, `'middle'`, `'bottom'`, `'alphabetic'`

## Transforms
- [ ] `save()` / `restore()` — also covers transform state
- [ ] `translate(x, y)`
- [ ] `rotate(angle)`
- [ ] `scale(x, y)`
- [ ] `setTransform(a, b, c, d, e, f)`
- [ ] `resetTransform()`

## Gradients *(nice to have)*
- [ ] `createLinearGradient(x0, y0, x1, y1)`
- [ ] `createRadialGradient(x0, y0, r0, x1, y1, r1)`
- [ ] `gradient.addColorStop(offset, color)`

## Examples
- [] `Create examples of all the primitives from the Canvas API docs`
