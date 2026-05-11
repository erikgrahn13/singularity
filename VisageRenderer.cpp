#include "VisageRenderer.h"
#include <visage_graphics/tests/lato_regular.h>

#include <cmath>
#include <algorithm>
#include <cctype>

static constexpr float kPi = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static visage::Color parseColorString(const std::string& color)
{
    if (color.empty())
        return visage::Color(0xff000000u);

    if (color[0] == '#')
        return visage::Color::fromHexString(color);

    // Basic CSS rgb/rgba
    if (color.substr(0, 4) == "rgba" || color.substr(0, 3) == "rgb") {
        auto start = color.find('(');
        auto end   = color.find(')');
        if (start == std::string::npos || end == std::string::npos)
            return visage::Color(0xff000000u);
        std::string inner = color.substr(start + 1, end - start - 1);
        float r = 0, g = 0, b = 0, a = 1.0f;
        int i = 0;
        std::string token;
        for (char ch : inner + ',') {
            if (ch == ',') {
                float v = std::stof(token);
                if (i == 0) r = v / 255.0f;
                else if (i == 1) g = v / 255.0f;
                else if (i == 2) b = v / 255.0f;
                else if (i == 3) a = v;
                token.clear(); ++i;
            } else if (ch != ' ') token += ch;
        }
        return visage::Color(a, r, g, b);
    }

    // Fallback: black
    return visage::Color(0xff000000u);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

VisageRenderer::VisageRenderer(int width, int height)
    : width_(width), height_(height) {}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void VisageRenderer::clear()
{
    if (!canvas_) return;
    canvas_->setColor(visage::Color(0xff000000u));
    canvas_->fill(0, 0, width_, height_);
    resetFrame();
}

void VisageRenderer::resetFrame()
{
    // Safety: close any layers JS forgot to close
    while (!layerStack_.empty()) {
        canvas_->endRegion();
        layerStack_.pop_back();
    }

    state_ = DrawState{};
    stateStack_.clear();
    gradients_.clear();
    if (canvas_) canvas_->setBlendMode(visage::BlendMode::Alpha);
}

void VisageRenderer::save()
{
    stateStack_.push_back(state_);
}

void VisageRenderer::restore()
{
    if (stateStack_.empty()) return;
    state_ = stateStack_.back();
    stateStack_.pop_back();
    if (canvas_) canvas_->setBlendMode(state_.blendMode);
}

void VisageRenderer::setGlobalAlpha(float alpha)
{
    state_.globalAlpha = alpha;
}

void VisageRenderer::translate(float x, float y)
{
    state_.translateX += x;
    state_.translateY += y;
}

void VisageRenderer::rotate(float angle)
{
    state_.rotateDeg += angle * (180.0f / kPi);
}

void VisageRenderer::scale(float x, float y)
{
    state_.scaleX *= x;
    state_.scaleY *= y;
}

void VisageRenderer::resetTransform()
{
    state_.translateX = 0;
    state_.translateY = 0;
    state_.rotateDeg  = 0;
    state_.scaleX     = 1;
    state_.scaleY     = 1;
}

// ---------------------------------------------------------------------------
// Color / Gradient
// ---------------------------------------------------------------------------

visage::Color VisageRenderer::parseColor(const std::string& color) const
{
    visage::Color c = parseColorString(color);
    // Apply global alpha
    return visage::Color(c.alpha() * state_.globalAlpha, c.red(), c.green(), c.blue());
}

void VisageRenderer::setFillStyle(const std::string& color)
{
    state_.fillColor     = parseColorString(color);
    state_.fillGradientId = -1;
}

void VisageRenderer::setStrokeStyle(const std::string& color)
{
    state_.strokeColor = parseColorString(color);
}

void VisageRenderer::setLineWidth(float lineWidth)
{
    state_.lineWidth = lineWidth;
}

void VisageRenderer::setLineCap(const std::string& cap)  { state_.lineCap  = cap; }
void VisageRenderer::setLineJoin(const std::string& join) { state_.lineJoin = join; }
void VisageRenderer::setLineDash(const std::vector<float>& segments) { state_.lineDash = segments; }
void VisageRenderer::setLineDashOffset(float offset) { state_.lineDashOffset = offset; }
void VisageRenderer::setHdrMultiplier(float mult) { state_.hdrMultiplier = mult; }

int VisageRenderer::createLinearGradient(float x0, float y0, float x1, float y1)
{
    GradientData g;
    g.type = GradientData::Type::Linear;
    g.x0 = x0; g.y0 = y0; g.x1 = x1; g.y1 = y1;
    gradients_.push_back(std::move(g));
    return static_cast<int>(gradients_.size()) - 1;
}

int VisageRenderer::createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1)
{
    GradientData g;
    g.type = GradientData::Type::Radial;
    g.x0 = x0; g.y0 = y0; g.r0 = r0;
    g.x1 = x1; g.y1 = y1; g.r1 = r1;
    gradients_.push_back(std::move(g));
    return static_cast<int>(gradients_.size()) - 1;
}

void VisageRenderer::addColorStop(int id, float offset, const std::string& color)
{
    if (id < 0 || id >= static_cast<int>(gradients_.size())) return;
    gradients_[id].stops.push_back({ offset, parseColorString(color) });
}

void VisageRenderer::setFillStyleGradient(int i)
{
    state_.fillGradientId = i;
}

void VisageRenderer::setStrokeStyleGradient(int i)
{
    state_.strokeGradientId = i;
}

visage::Brush VisageRenderer::makeFillBrush() const
{
    auto applyHdr = [&](visage::Color c) {
        c.setHdr(state_.hdrMultiplier);
        return c;
    };

    if (state_.fillGradientId >= 0 &&
        state_.fillGradientId < static_cast<int>(gradients_.size()))
    {
        const GradientData& g = gradients_[state_.fillGradientId];
        visage::Gradient grad;
        for (const auto& stop : g.stops)
            grad.addColorStop(applyHdr(stop.second), stop.first);

        if (grad.numColors() == 0)
            return visage::Brush::solid(applyHdr(state_.fillColor));

        if (g.type == GradientData::Type::Linear) {
            return visage::Brush::linear(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY),
                visage::Point(g.x1 + state_.translateX, g.y1 + state_.translateY));
        } else {
            float radius = std::max(g.r0, g.r1);
            return visage::Brush::radial(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY), radius);
        }
    }

    visage::Color c = state_.fillColor;
    c = visage::Color(c.alpha() * state_.globalAlpha, c.red(), c.green(), c.blue());
    c.setHdr(state_.hdrMultiplier);
    return visage::Brush::solid(c);
}

visage::Brush VisageRenderer::makeStrokeBrush() const
{
    auto applyHdr = [&](visage::Color c) {
        c.setHdr(state_.hdrMultiplier);
        return c;
    };

    if (state_.strokeGradientId >= 0 &&
        state_.strokeGradientId < static_cast<int>(gradients_.size()))
    {
        const GradientData& g = gradients_[state_.strokeGradientId];
        visage::Gradient grad;
        for (const auto& stop : g.stops)
            grad.addColorStop(applyHdr(stop.second), stop.first);

        if (grad.numColors() == 0) {
            visage::Color c = state_.strokeColor;
            c = visage::Color(c.alpha() * state_.globalAlpha, c.red(), c.green(), c.blue());
            c.setHdr(state_.hdrMultiplier);
            return visage::Brush::solid(c);
        }

        if (g.type == GradientData::Type::Linear) {
            return visage::Brush::linear(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY),
                visage::Point(g.x1 + state_.translateX, g.y1 + state_.translateY));
        } else {
            float radius = std::max(g.r0, g.r1);
            return visage::Brush::radial(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY), radius);
        }
    }

    visage::Color c = state_.strokeColor;
    c = visage::Color(c.alpha() * state_.globalAlpha, c.red(), c.green(), c.blue());
    c.setHdr(state_.hdrMultiplier);
    return visage::Brush::solid(c);
}

// ---------------------------------------------------------------------------
// Rectangles
// ---------------------------------------------------------------------------

void VisageRenderer::fillRect(float x, float y, float width, float height)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->fill(x + state_.translateX, y + state_.translateY, width, height);
}

void VisageRenderer::clearRect(float x, float y, float width, float height)
{
    if (!canvas_) return;
    canvas_->setColor(visage::Color(0x00000000u));
    canvas_->fill(x + state_.translateX, y + state_.translateY, width, height);
}

void VisageRenderer::strokeRect(float x, float y, float width, float height)
{
    if (!canvas_) return;
    canvas_->setColor(makeStrokeBrush());
    canvas_->rectangleBorder(x + state_.translateX, y + state_.translateY,
                              width, height, state_.lineWidth);
}

void VisageRenderer::roundRect(float x, float y, float width, float height, float radii)
{
    // Accumulates as a path sub-shape
    currentPath_.addRoundedRectangle(x + state_.translateX, y + state_.translateY,
                                     width, height, radii);
}

// ---------------------------------------------------------------------------
// Path
// ---------------------------------------------------------------------------

void VisageRenderer::beginPath()
{
    currentPath_.clear();
}

void VisageRenderer::moveTo(float x, float y)
{
    currentPath_.moveTo(x + state_.translateX, y + state_.translateY);
}

void VisageRenderer::lineTo(float x, float y)
{
    currentPath_.lineTo(x + state_.translateX, y + state_.translateY);
}

void VisageRenderer::closePath()
{
    currentPath_.close();
}

void VisageRenderer::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    float tx = state_.translateX, ty = state_.translateY;
    currentPath_.quadraticTo(cpx + tx, cpy + ty, x + tx, y + ty);
}

void VisageRenderer::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                                   float x, float y)
{
    float tx = state_.translateX, ty = state_.translateY;
    currentPath_.bezierTo(cp1x + tx, cp1y + ty, cp2x + tx, cp2y + ty, x + tx, y + ty);
}

void VisageRenderer::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    // Convert arcTo(p1, p2, r) — visage Path doesn't have arcTo directly,
    // so approximate with the SVG arc path command via two line + arc segments.
    // Simple approximation: just line to the midpoint corner (loses the arc curve).
    // For full fidelity a proper arcTo algorithm is needed, but this covers most UI cases.
    float tx = state_.translateX, ty = state_.translateY;
    currentPath_.lineTo(x1 + tx, y1 + ty);
    currentPath_.lineTo(x2 + tx, y2 + ty);
}

void VisageRenderer::ellipse(float x, float y, float radiusX, float radiusY,
                             float /*rotation*/, float startAngle, float endAngle)
{
    // Approximate with addEllipse for full circles, bezier for arcs
    if (std::abs(endAngle - startAngle) >= 2.0f * kPi - 0.001f) {
        currentPath_.addEllipse(x + state_.translateX, y + state_.translateY, radiusX, radiusY);
    } else {
        // Approximate arc with bezier segments
        float sweep = endAngle - startAngle;
        int segments = std::max(1, static_cast<int>(std::abs(sweep) / (kPi / 2.0f)) + 1);
        float segAngle = sweep / segments;
        float cx = x + state_.translateX, cy = y + state_.translateY;

        float sx = cx + radiusX * std::cos(startAngle);
        float sy = cy + radiusY * std::sin(startAngle);
        currentPath_.moveTo(sx, sy);

        float alpha = std::sin(segAngle) * (std::sqrt(4.0f + 3.0f * std::tan(segAngle / 2.0f) *
                                             std::tan(segAngle / 2.0f)) - 1.0f) / 3.0f;
        float angle = startAngle;
        for (int i = 0; i < segments; ++i) {
            float ex = cx + radiusX * std::cos(angle + segAngle);
            float ey = cy + radiusY * std::sin(angle + segAngle);
            float dx1 = cx + radiusX * std::cos(angle);
            float dy1 = cy + radiusY * std::sin(angle);
            currentPath_.bezierTo(
                dx1 - alpha * radiusX * std::sin(angle), dy1 + alpha * radiusY * std::cos(angle),
                ex + alpha * radiusX * std::sin(angle + segAngle),
                ey - alpha * radiusY * std::cos(angle + segAngle),
                ex, ey);
            angle += segAngle;
        }
    }
}

void VisageRenderer::rect(float x, float y, float width, float height)
{
    currentPath_.addRectangle(x + state_.translateX, y + state_.translateY, width, height);
}

void VisageRenderer::arc(float x, float y, float radius, float startAngle, float endAngle)
{
    ellipse(x, y, radius, radius, 0.0f, startAngle, endAngle);
}

void VisageRenderer::fill()
{
    if (!canvas_ || currentPath_.numPoints() == 0) return;
    canvas_->setColor(makeFillBrush());
    canvas_->fill(currentPath_, 0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_));
}

void VisageRenderer::stroke()
{
    if (!canvas_ || currentPath_.numPoints() == 0) return;
    canvas_->setColor(makeStrokeBrush());

    auto mapCap = [](const std::string& s) {
        if (s == "round") return visage::Path::EndCap::Round;
        if (s == "square") return visage::Path::EndCap::Square;
        return visage::Path::EndCap::Butt;
    };
    auto mapJoin = [](const std::string& s) {
        if (s == "round") return visage::Path::Join::Round;
        if (s == "bevel") return visage::Path::Join::Bevel;
        return visage::Path::Join::Miter;
    };

    visage::Path stroked = currentPath_.stroke(
        state_.lineWidth,
        mapJoin(state_.lineJoin),
        mapCap(state_.lineCap),
        state_.lineDash,
        state_.lineDashOffset);
    canvas_->fill(stroked, 0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_));
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

static visage::Font makeFont(float size)
{
    // fontFamily_ from canvas2D JS (e.g. "Arial") cannot be used as a file path.
    // Fall back to the embedded Lato Regular that ships with visage.
    return visage::Font(size,
                        fonts::Lato_Regular_ttf_data,
                        static_cast<int>(sizeof(fonts::Lato_Regular_ttf_data)));
}

void VisageRenderer::font(const std::string& text)
{
    auto pxPos = text.find("px");
    if (pxPos == std::string::npos) return;

    size_t sizeStart = pxPos;
    while (sizeStart > 0 &&
           (std::isdigit((unsigned char)text[sizeStart - 1]) || text[sizeStart - 1] == '.'))
        --sizeStart;

    fontSize_ = std::stof(text.substr(sizeStart, pxPos - sizeStart));
    fontFamily_ = (pxPos + 3 <= text.size()) ? text.substr(pxPos + 3) : "";
}

void VisageRenderer::textAlign(const std::string& align)
{
    state_.textAlignStr = align;
}

void VisageRenderer::textBaseline(const std::string& baseline)
{
    state_.textBaselineStr = baseline;
}

void VisageRenderer::fillText(const std::string& text, float x, float y)
{
    if (!canvas_ || text.empty()) return;

    visage::Font::Justification just = visage::Font::kLeft;
    if (state_.textAlignStr == "center")      just = visage::Font::kCenter;
    else if (state_.textAlignStr == "right" ||
             state_.textAlignStr == "end")    just = visage::Font::kRight;

    // Vertical alignment: shift y by approximate line metrics
    float lineH = fontSize_;
    float drawY = y + state_.translateY;
    if (state_.textBaselineStr == "top")           drawY -= 0.0f;
    else if (state_.textBaselineStr == "middle")   drawY -= lineH * 0.5f;
    else if (state_.textBaselineStr == "bottom")   drawY -= lineH;
    else /* alphabetic */                          drawY -= lineH * 0.8f;

    float drawX = x + state_.translateX;

    visage::Font vFont = makeFont(fontSize_);
    canvas_->setColor(makeFillBrush());

    // In canvas2D, x is the anchor point whose meaning depends on textAlign:
    //   left   → x is the left edge   → visage box: (drawX, *, width_-drawX, *)
    //   center → x is the center      → visage box: (0,     *, 2*drawX,       *)
    //   right  → x is the right edge  → visage box: (0,     *, drawX,          *)
    float boxX, boxW;
    if (just == visage::Font::kCenter) {
        boxX = 0;      boxW = 2.0f * drawX;
    } else if (just == visage::Font::kRight) {
        boxX = 0;      boxW = drawX;
    } else {
        boxX = drawX;  boxW = static_cast<float>(width_) - drawX;
    }

    canvas_->text(visage::String(text), vFont, just, boxX, drawY, boxW, lineH);
}

void VisageRenderer::strokeText(const std::string& text, float x, float y)
{
    // Visage has no stroke-text primitive; fall back to fill with stroke color
    if (!canvas_ || text.empty()) return;

    visage::Font::Justification just = visage::Font::kLeft;
    if (state_.textAlignStr == "center")      just = visage::Font::kCenter;
    else if (state_.textAlignStr == "right" ||
             state_.textAlignStr == "end")    just = visage::Font::kRight;

    float lineH = fontSize_;
    float drawY = y + state_.translateY;
    if (state_.textBaselineStr == "top")           drawY -= 0.0f;
    else if (state_.textBaselineStr == "middle")   drawY -= lineH * 0.5f;
    else if (state_.textBaselineStr == "bottom")   drawY -= lineH;
    else                                           drawY -= lineH * 0.8f;

    float drawX = x + state_.translateX;

    visage::Font vFont = makeFont(fontSize_);
    canvas_->setColor(makeStrokeBrush());

    float boxX, boxW;
    if (just == visage::Font::kCenter) {
        boxX = 0;      boxW = 2.0f * drawX;
    } else if (just == visage::Font::kRight) {
        boxX = 0;      boxW = drawX;
    } else {
        boxX = drawX;  boxW = static_cast<float>(width_) - drawX;
    }

    canvas_->text(visage::String(text), vFont, just, boxX, drawY, boxW, lineH);
}

float VisageRenderer::measureText(const std::string& text)
{
    visage::Font vFont = makeFont(fontSize_);
    std::u32string u32 = visage::String::convertUtf8ToUtf32<std::u32string>(text);
    return vFont.stringWidth(u32.c_str(), static_cast<int>(u32.size()));
}

// ---------------------------------------------------------------------------
// Images
// ---------------------------------------------------------------------------

void VisageRenderer::registerImage(const std::string& name, const uint8_t* data, size_t size)
{
    registeredImages_[name] = std::vector<uint8_t>(data, data + size);
}

void VisageRenderer::drawImage(const std::string& name, float dx, float dy, float dw, float dh)
{
    if (!canvas_) return;

    // Ensure the image bytes are in the cache, loading from disk if needed.
    if (registeredImages_.find(name) == registeredImages_.end()) {
        auto tryLoad = [&](const std::string& path) -> bool {
            FILE* f = fopen(path.c_str(), "rb");
            if (!f) return false;
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            rewind(f);
            if (sz <= 0) { fclose(f); return false; }
            std::vector<uint8_t> buf(sz);
            fread(buf.data(), 1, sz, f);
            fclose(f);
            registeredImages_[name] = std::move(buf);
            return true;
        };

        if (!tryLoad(name)) {
            std::string fullPath = std::string(UI_DIR) + "/" + name;
            if (!tryLoad(fullPath)) {
                printf("[VisageRenderer::drawImage] failed to load: %s\n", name.c_str());
                return;
            }
        }
    }

    const auto& bytes = registeredImages_[name];
    canvas_->setColor(0xffffffff);
    canvas_->image(bytes.data(), static_cast<int>(bytes.size()),
                   dx + state_.translateX, dy + state_.translateY,
                   dw, dh);
}

// ---------------------------------------------------------------------------
// Visage-native GPU primitives
// ---------------------------------------------------------------------------

void VisageRenderer::circle(float cx, float cy, float radius)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    // visage circle: top-left x,y + diameter
    canvas_->circle(cx - radius + state_.translateX, cy - radius + state_.translateY, radius * 2.0f);
}

void VisageRenderer::fadeCircle(float cx, float cy, float radius, float pixelWidth)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->fadeCircle(cx - radius + state_.translateX, cy - radius + state_.translateY,
                        radius * 2.0f, pixelWidth);
}

void VisageRenderer::ring(float x, float y, float diameter, float thickness)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->ring(x + state_.translateX, y + state_.translateY, diameter, thickness);
}

void VisageRenderer::squircle(float x, float y, float diameter, float power)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->squircle(x + state_.translateX, y + state_.translateY, diameter, power);
}

void VisageRenderer::roundedArc(float x, float y, float diameter, float thickness,
                                float centerRadians, float arcRadians)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->roundedArc(x + state_.translateX, y + state_.translateY,
                        diameter, thickness, centerRadians, arcRadians);
}

void VisageRenderer::flatArc(float x, float y, float diameter, float thickness,
                             float centerRadians, float arcRadians)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->flatArc(x + state_.translateX, y + state_.translateY,
                     diameter, thickness, centerRadians, arcRadians);
}

void VisageRenderer::segment(float ax, float ay, float bx, float by, float thickness)
{
    if (!canvas_) return;
    canvas_->setColor(makeStrokeBrush());
    canvas_->segment(ax + state_.translateX, ay + state_.translateY,
                     bx + state_.translateX, by + state_.translateY,
                     thickness, true);
}

void VisageRenderer::triangle(float ax, float ay, float bx, float by, float cx, float cy)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->triangle(ax + state_.translateX, ay + state_.translateY,
                      bx + state_.translateX, by + state_.translateY,
                      cx + state_.translateX, cy + state_.translateY);
}

void VisageRenderer::diamond(float x, float y, float diameter)
{
    if (!canvas_) return;
    canvas_->setColor(makeFillBrush());
    canvas_->diamond(x + state_.translateX, y + state_.translateY, diameter, 0.0f);
}

void VisageRenderer::setBlendMode(const std::string& mode)
{
    visage::BlendMode bm = visage::BlendMode::Alpha;
    if      (mode == "add")         bm = visage::BlendMode::Add;
    else if (mode == "sub")         bm = visage::BlendMode::Sub;
    else if (mode == "mult")        bm = visage::BlendMode::Mult;
    else if (mode == "opaque")      bm = visage::BlendMode::Opaque;
    else if (mode == "composite")   bm = visage::BlendMode::Composite;
    else if (mode == "maskAdd")     bm = visage::BlendMode::MaskAdd;
    else if (mode == "maskRemove")  bm = visage::BlendMode::MaskRemove;
    state_.blendMode = bm;
    if (canvas_) canvas_->setBlendMode(bm);
}

void VisageRenderer::beginLayer(float opacity)
{
    if (!canvas_) return;
    int depth = static_cast<int>(layerStack_.size());

    // Grow pool if needed — regions are persistent (registered with canvas once, reused every frame)
    if (depth >= static_cast<int>(layerPool_.size())) {
        auto reg = std::make_unique<visage::Region>();
        canvas_->addRegion(reg.get());          // adds to default_region_, sets canvas ptr
        reg->setBounds(0, 0, width_, height_);  // must be BEFORE setNeedsLayer
        reg->setNeedsLayer(true);               // creates intermediate GPU layer
        layerPool_.push_back(std::move(reg));
    }

    layerStack_.push_back({ opacity });
    canvas_->beginRegion(layerPool_[depth].get());  // redirects all draw calls into this region
}

void VisageRenderer::endLayer()
{
    if (!canvas_ || layerStack_.empty()) return;
    float opacity = layerStack_.back().opacity;
    layerStack_.pop_back();

    // Composite the entire layer at 'opacity' via Mult blend (same as Frame::setAlphaTransparency)
    if (opacity < 1.0f) {
        canvas_->setBlendMode(visage::BlendMode::Mult);
        canvas_->setColor(visage::Color(0xffffffff).withAlpha(opacity));
        canvas_->fill(0, 0, width_, height_);
    }
    canvas_->endRegion();
    // Restore the VisageRenderer's current blend mode
    canvas_->setBlendMode(state_.blendMode);
}

std::unique_ptr<IRenderer> createRenderer(int width, int height)
{
    return std::make_unique<VisageRenderer>(width, height);
}