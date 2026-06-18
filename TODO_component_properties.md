# Component Properties TODO

## Layout (High Priority)

- [x] `flexDirection` — `"row"` | `"column"` — controls main axis for children
- [x] `gap` — spacing between children (uniform)
- [x] `alignItems` — `"flex-start"` | `"center"` | `"flex-end"` — cross-axis alignment
- [x] `justifyContent` — `"flex-start"` | `"center"` | `"flex-end"` | `"space-between"` | `"space-around"` | `"space-evenly"` — main-axis distribution
- [x] `padding` — inner spacing (uniform number for now)
- [x] `margin` — outer spacing, shifts position inward and shrinks rendered size
- [ ] `wrap` — boolean, allow children to wrap to next row/column

## Visual

- [ ] `opacity` — 0.0–1.0, alpha of entire component
- [ ] `visible` — boolean, hide without destroying
- [x] `borderRadius` — rounded corners
- [x] `border` — `{ width, color }`
## Interaction

- [ ] `onClick` — tap/click handler
- [ ] `onHover` — hover state callback
- [ ] `disabled` — boolean, grey out + block input
- [ ] `cursor` — pointer, grab, etc.

## Transform / Animation

- [ ] `transform` — `{ scale, rotate, translateX, translateY }`
- [ ] `transition` — animate property changes over time

## Identity

- [ ] `id` / `key` — stable identity for reconciliation
