#include <memory>
#include "VisageRenderer2.h"
#include <visage_graphics/tests/lato_regular.h>
#include <visage_graphics/post_effects.h>
#include <cmath>
#include <algorithm>
#include <iostream>

static constexpr float kPi = 3.14159265358979323846f;

static visage::Font makeFont(float size) {
    return visage::Font(size,
                        fonts::Lato_Regular_ttf_data,
                        static_cast<int>(sizeof(fonts::Lato_Regular_ttf_data)),
                        1.0f);
}

std::unique_ptr<IRenderer> IRenderer::createRenderer(void* parentHandle)
{
    return std::make_unique<VisageRenderer>(parentHandle);
}

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

// Apply an HDR multiplier to a color (values >1 drive bloom/glow).
static visage::Color withHdr(visage::Color c, float hdr) {
    c.setHdr(hdr);
    return c;
}

VisageRenderer::VisageRenderer(void *parentHandle)
{
    rootFrame_ = static_cast<visage::ApplicationWindow*>(parentHandle);
    rootFrame_->setDpiScale(visage::defaultDpiScale());

    rootFrame_->onDraw() += [this](visage::Canvas& canvas) {
        // canvas.setColor(0xff0000ff);
        canvas.fill(0, 0, rootFrame_->width(), rootFrame_->height());
    };
}

void *VisageRenderer::getRootComponent()
{
    return rootFrame_;
}

void *VisageRenderer::createComponent(void *parentComponent)
{
    auto* parentFrame = static_cast<visage::Frame*>(parentComponent);
    auto child = std::make_unique<visage::Frame>();
    auto* childPtr = child.get();
    
    childPtr->onMouseDown() += [this, childPtr](const visage::MouseEvent& e) {
        if (componentMouseDownCallback_) {
            componentMouseDownCallback_(childPtr, e.position.x, e.position.y);
        }
    };

    childPtr->onMouseUp() += [this, childPtr](const visage::MouseEvent& e) {
        if (componentMouseUpCallback_) {
            componentMouseUpCallback_(childPtr, e.position.x, e.position.y);
        }
    };
    childPtr->onMouseDrag() += [this, childPtr](const visage::MouseEvent& e) {
        if (componentMouseDragCallback_) {
            componentMouseDragCallback_(childPtr, e.position.x, e.position.y);
        }
    };
    childPtr->onMouseEnter() += [this, childPtr](const visage::MouseEvent& e) {
        if (componentMouseEnterCallback_)
            componentMouseEnterCallback_(childPtr);
    };
    childPtr->onMouseExit() += [this, childPtr](const visage::MouseEvent& e) {
        if (componentMouseExitCallback_)
            componentMouseExitCallback_(childPtr);
    };
    childPtr->onMouseWheel() += [this, childPtr](const visage::MouseEvent& e) {
        if (componentMouseWheelCallback_)
            componentMouseWheelCallback_(childPtr, e.wheel_delta_x, e.wheel_delta_y);
        return false;
    };
    
    parentFrame->addChild(std::move(child));
    

    return childPtr;
}

void VisageRenderer::setBounds(void *component, float x, float y, float w, float h)
{
    auto* frame = static_cast<visage::Frame*>(component);
    frame->setBounds(x, y, w, h);
}

void VisageRenderer::setDrawCallback(void *component, std::function<void(void *canvas)> cb)
{
    auto* frame = static_cast<visage::Frame*>(component);

    frame->onDraw() = [cb, frame](visage::Canvas& canvas) {
        cb(&canvas);
    };
}

void VisageRenderer::clear()
{
    while (!rootFrame_->children().empty())
        rootFrame_->removeChild(rootFrame_->children().back());
    gradients_.clear();
    state_ = DrawState{};
    stateStack_.clear();
    rootFrame_->redraw();
}

void VisageRenderer::fillRect(void *canvas, float x, float y, float w, float h)
{
    auto* c = static_cast<visage::Canvas*>(canvas);

    if (state_.fillGradientId >= 0 && state_.fillGradientId < static_cast<int>(gradients_.size())) {
        const auto& g = gradients_[state_.fillGradientId];
        visage::Gradient grad;
        for (const auto& stop : g.stops)
            grad.addColorStop(stop.second, stop.first); // HDR is per-stop, stored in color

        if (grad.numColors() == 0) {
            c->setColor(withHdr(state_.fillColor, state_.hdrMultiplier));
        } else if (g.type == GradientData::Type::Linear) {
            c->setColor(visage::Brush::linear(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY),
                visage::Point(g.x1 + state_.translateX, g.y1 + state_.translateY)));
        } else {
            float radius = std::max(g.r0, g.r1);
            c->setColor(visage::Brush::radial(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY), radius));
        }
    } else {
        c->setColor(withHdr(state_.fillColor, state_.hdrMultiplier));
    }
    c->fill(x + state_.translateX, y + state_.translateY, w, h);
}

void VisageRenderer::strokeRect(void *canvas, float x, float y, float w, float h)
{
    auto* c = static_cast<visage::Canvas*>(canvas);
    c->setColor(withHdr(state_.strokeColor, state_.hdrMultiplier));
    c->rectangleBorder(x + state_.translateX, y + state_.translateY, w, h, state_.lineWidth);
}

void VisageRenderer::clearRect(void *canvas, float x, float y, float width, float height)
{
    auto* c = static_cast<visage::Canvas*>(canvas);
    c->setColor(visage::Color(0x00000000u));
    c->fill(x + state_.translateX, y + state_.translateY, width, height);
}

// ---------------------------------------------------------------------------
// Style setters
// ---------------------------------------------------------------------------

void VisageRenderer::setFillStyle(void* canvas, const std::string& color)
{
    state_.fillColor = parseColorString(color);
    state_.fillGradientId = -1;
}

void VisageRenderer::setStrokeStyle(void* canvas, const std::string& color)
{
    state_.strokeColor = parseColorString(color);
    state_.strokeGradientId = -1;
}

void VisageRenderer::setLineWidth(void* canvas, float width)
{
    state_.lineWidth = width;
}

void VisageRenderer::setFont(void* canvas, const std::string& font)
{
    auto pxPos = font.find("px");
    if (pxPos != std::string::npos) {
        size_t sizeStart = pxPos;
        while (sizeStart > 0 &&
               (std::isdigit((unsigned char)font[sizeStart - 1]) || font[sizeStart - 1] == '.'))
            --sizeStart;
        state_.fontSize = std::stof(font.substr(sizeStart, pxPos - sizeStart));
    }
}

void VisageRenderer::setGlobalAlpha(void* canvas, float alpha)
{
    state_.globalAlpha = std::max(0.0f, std::min(1.0f, alpha));
}

void VisageRenderer::setTextAlign(void* canvas, const std::string& align)
{
    state_.textAlign = align;
}

void VisageRenderer::setTextBaseline(void* canvas, const std::string& baseline)
{
    state_.textBaseline = baseline;
}

// Gradient APIs
int VisageRenderer::createLinearGradient(void* canvas, float x0, float y0, float x1, float y1)
{
    GradientData g;
    g.type = GradientData::Type::Linear;
    g.x0 = x0; g.y0 = y0; g.x1 = x1; g.y1 = y1;
    gradients_.push_back(std::move(g));
    return static_cast<int>(gradients_.size()) - 1;
}

int VisageRenderer::createRadialGradient(void* canvas, float x0, float y0, float r0, float x1, float y1, float r1)
{
    GradientData g;
    g.type = GradientData::Type::Radial;
    g.x0 = x0; g.y0 = y0; g.r0 = r0; g.x1 = x1; g.y1 = y1; g.r1 = r1;
    gradients_.push_back(std::move(g));
    return static_cast<int>(gradients_.size()) - 1;
}

void VisageRenderer::addColorStop(void* canvas, int id, float offset, const std::string& color, float hdr)
{
    if (id < 0 || id >= static_cast<int>(gradients_.size())) return;
    visage::Color c = parseColorString(color);
    c.setHdr(hdr);
    gradients_[id].stops.push_back({ offset, c });
}

void VisageRenderer::setFillStyleGradient(void* canvas, int i)
{
    state_.fillGradientId = i;
}

void VisageRenderer::setStrokeStyleGradient(void* canvas, int i)
{
    state_.strokeGradientId = i;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void VisageRenderer::save(void* canvas)
{
    stateStack_.push_back(state_);
}

void VisageRenderer::restore(void* canvas)
{
    if (!stateStack_.empty()) {
        state_ = stateStack_.back();
        stateStack_.pop_back();
    }
}

// ---------------------------------------------------------------------------
// Transforms
// ---------------------------------------------------------------------------

void VisageRenderer::translate(void* canvas, float x, float y)
{
    state_.translateX += x;
    state_.translateY += y;
}

void VisageRenderer::rotate(void* canvas, float angle)
{
    // Rotation is stored but not applied to immediate draws (path ops use it via translate).
}

void VisageRenderer::scale(void* canvas, float x, float y)
{
    state_.scaleX *= x;
    state_.scaleY *= y;
}

void VisageRenderer::resetTransform(void* canvas)
{
    state_.translateX = 0.0f;
    state_.translateY = 0.0f;
    state_.scaleX = 1.0f;
    state_.scaleY = 1.0f;
}

// ---------------------------------------------------------------------------
// Path building
// ---------------------------------------------------------------------------

void VisageRenderer::beginPath(void* canvas)
{
    currentPath_.clear();
}

void VisageRenderer::moveTo(void* canvas, float x, float y)
{
    currentPath_.moveTo(x + state_.translateX, y + state_.translateY);
}

void VisageRenderer::lineTo(void* canvas, float x, float y)
{
    currentPath_.lineTo(x + state_.translateX, y + state_.translateY);
}

void VisageRenderer::closePath(void* canvas)
{
    currentPath_.close();
}

void VisageRenderer::quadraticCurveTo(void* canvas, float cpx, float cpy, float x, float y)
{
    float tx = state_.translateX, ty = state_.translateY;
    currentPath_.quadraticTo(cpx + tx, cpy + ty, x + tx, y + ty);
}

void VisageRenderer::bezierCurveTo(void* canvas, float cp1x, float cp1y,
                                    float cp2x, float cp2y, float x, float y)
{
    float tx = state_.translateX, ty = state_.translateY;
    currentPath_.bezierTo(cp1x + tx, cp1y + ty, cp2x + tx, cp2y + ty, x + tx, y + ty);
}

void VisageRenderer::ellipse(void* canvas,
                             float cx,
                             float cy,
                             float rx,
                             float ry,
                             float /*rotation*/,
                             float startAngle,
                             float endAngle,
                             bool anticlockwise)
{
    float tx = state_.translateX;
    float ty = state_.translateY;

    float pcx = cx + tx;
    float pcy = cy + ty;

    float fullCircle = 2.0f * kPi;

    if (std::abs(endAngle - startAngle) >= fullCircle - 0.001f) {
        currentPath_.addEllipse(pcx, pcy, rx, ry);
        return;
    }

    float sweep = endAngle - startAngle;

    if (!anticlockwise && sweep < 0.0f)
        sweep += fullCircle;

    if (anticlockwise && sweep > 0.0f)
        sweep -= fullCircle;

    if (sweep > fullCircle)
        sweep = fullCircle;

    if (sweep < -fullCircle)
        sweep = -fullCircle;

    int segments = std::max(
        1,
        static_cast<int>(std::ceil(std::abs(sweep) / (kPi / 2.0f)))
    );

    float delta = sweep / segments;

    float startX = pcx + rx * std::cos(startAngle);
    float startY = pcy + ry * std::sin(startAngle);

    if (currentPath_.numPoints() == 0)
        currentPath_.moveTo(startX, startY);
    else
        currentPath_.lineTo(startX, startY);

    float angle = startAngle;

    for (int i = 0; i < segments; ++i) {
        float next = angle + delta;
        float t = (4.0f / 3.0f) * std::tan((next - angle) / 4.0f);

        float x1 = pcx + rx * std::cos(angle);
        float y1 = pcy + ry * std::sin(angle);

        float x2 = pcx + rx * std::cos(next);
        float y2 = pcy + ry * std::sin(next);

        float dx1 = -rx * std::sin(angle);
        float dy1 =  ry * std::cos(angle);

        float dx2 = -rx * std::sin(next);
        float dy2 =  ry * std::cos(next);

        currentPath_.bezierTo(
            x1 + t * dx1,
            y1 + t * dy1,
            x2 - t * dx2,
            y2 - t * dy2,
            x2,
            y2
        );

        angle = next;
    }
}

void VisageRenderer::arc(void* canvas, float cx, float cy, float radius,
                          float startAngle, float endAngle, bool anticlockwise)
{
    ellipse(canvas, cx, cy, radius, radius, 0.0f, startAngle, endAngle, anticlockwise);
}

void VisageRenderer::arcTo(void* canvas, float x1, float y1, float x2, float y2, float /*radius*/)
{
    // Approximate with two lineTo segments (simplified)
    float tx = state_.translateX, ty = state_.translateY;
    currentPath_.lineTo(x1 + tx, y1 + ty);
    currentPath_.lineTo(x2 + tx, y2 + ty);
}

void VisageRenderer::rect(void* canvas, float x, float y, float w, float h)
{
    currentPath_.addRectangle(x + state_.translateX, y + state_.translateY, w, h);
}

void VisageRenderer::roundRect(void* canvas, float x, float y, float w, float h, float r)
{
    currentPath_.addRoundedRectangle(x + state_.translateX, y + state_.translateY, w, h, r);
}

// ---------------------------------------------------------------------------
// Path rendering
// ---------------------------------------------------------------------------

void VisageRenderer::fill(void* canvas)
{
    if (currentPath_.numPoints() == 0) return;
    auto* c = static_cast<visage::Canvas*>(canvas);

    if (state_.fillGradientId >= 0 && state_.fillGradientId < static_cast<int>(gradients_.size())) {
        const auto& g = gradients_[state_.fillGradientId];
        visage::Gradient grad;
        for (const auto& stop : g.stops)
            grad.addColorStop(stop.second, stop.first); // HDR is per-stop, stored in color

        if (grad.numColors() == 0) {
            c->setColor(withHdr(state_.fillColor, state_.hdrMultiplier));
        } else if (g.type == GradientData::Type::Linear) {
            c->setColor(visage::Brush::linear(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY),
                visage::Point(g.x1 + state_.translateX, g.y1 + state_.translateY)));
        } else {
            float radius = std::max(g.r0, g.r1);
            c->setColor(visage::Brush::radial(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY), radius));
        }
    } else {
        c->setColor(withHdr(state_.fillColor, state_.hdrMultiplier));
    }

    c->fill(currentPath_);
}

void VisageRenderer::stroke(void* canvas)
{
    if (currentPath_.numPoints() == 0)
        return;

    auto* c = static_cast<visage::Canvas*>(canvas);

    if (state_.strokeGradientId >= 0 && state_.strokeGradientId < static_cast<int>(gradients_.size())) {
        const auto& g = gradients_[state_.strokeGradientId];
        visage::Gradient grad;
        for (const auto& stop : g.stops)
            grad.addColorStop(stop.second, stop.first); // HDR is per-stop, stored in color

        if (grad.numColors() == 0) {
            c->setColor(withHdr(state_.strokeColor, state_.hdrMultiplier));
        } else if (g.type == GradientData::Type::Linear) {
            c->setColor(visage::Brush::linear(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY),
                visage::Point(g.x1 + state_.translateX, g.y1 + state_.translateY)));
        } else {
            float radius = std::max(g.r0, g.r1);
            c->setColor(visage::Brush::radial(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY), radius));
        }
    } else {
        c->setColor(withHdr(state_.strokeColor, state_.hdrMultiplier));
    }

    auto stroked = currentPath_.stroke(
        state_.lineWidth,
        visage::Path::Join::Round,
        visage::Path::EndCap::Round
    );
    c->fill(stroked);
    currentPath_.clear();
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

void VisageRenderer::fillText(void* canvas, const std::string& text, float x, float y)
{
    if (text.empty()) return;
    auto* c = static_cast<visage::Canvas*>(canvas);

    visage::Font::Justification just = visage::Font::kLeft;
    if (state_.textAlign == "center")                              just = visage::Font::kCenter;
    else if (state_.textAlign == "right" || state_.textAlign == "end") just = visage::Font::kRight;

    float lineH = state_.fontSize;
    float drawX = x + state_.translateX;
    float drawY = y + state_.translateY;

    if      (state_.textBaseline == "top")    { /* no shift */ }
    else if (state_.textBaseline == "middle") drawY -= lineH * 0.5f;
    else if (state_.textBaseline == "bottom") drawY -= lineH;
    else                                       drawY -= lineH * 0.8f; // alphabetic

    visage::Font vFont = makeFont(state_.fontSize);

    // Apply gradient if present
    if (state_.fillGradientId >= 0 && state_.fillGradientId < static_cast<int>(gradients_.size())) {
        const auto& g = gradients_[state_.fillGradientId];
        visage::Gradient grad;
        for (const auto& stop : g.stops)
            grad.addColorStop(stop.second, stop.first);

        if (g.type == GradientData::Type::Linear) {
            c->setColor(visage::Brush::linear(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY),
                visage::Point(g.x1 + state_.translateX, g.y1 + state_.translateY)));
        } else {
            float radius = std::max(g.r0, g.r1);
            c->setColor(visage::Brush::radial(grad,
                visage::Point(g.x0 + state_.translateX, g.y0 + state_.translateY), radius));
        }
    } else {
        c->setColor(state_.fillColor);
    }

    float boxX, boxW;
    if (just == visage::Font::kCenter) {
        boxX = 0.0f; boxW = 2.0f * drawX;
    } else if (just == visage::Font::kRight) {
        boxX = 0.0f; boxW = drawX;
    } else {
        boxX = drawX; boxW = static_cast<float>(c->width()) - drawX;
    }

    c->text(visage::String(text), vFont, just, boxX, drawY, boxW, lineH);
}

void VisageRenderer::strokeText(void* canvas, const std::string& text, float x, float y)
{
    // Visage has no stroke-text primitive; render with stroke colour instead
    if (text.empty()) return;
    auto* c = static_cast<visage::Canvas*>(canvas);

    visage::Font::Justification just = visage::Font::kLeft;
    if (state_.textAlign == "center")                              just = visage::Font::kCenter;
    else if (state_.textAlign == "right" || state_.textAlign == "end") just = visage::Font::kRight;

    float lineH = state_.fontSize;
    float drawX = x + state_.translateX;
    float drawY = y + state_.translateY;

    if      (state_.textBaseline == "top")    { /* no shift */ }
    else if (state_.textBaseline == "middle") drawY -= lineH * 0.5f;
    else if (state_.textBaseline == "bottom") drawY -= lineH;
    else                                       drawY -= lineH * 0.8f;

    visage::Font vFont = makeFont(state_.fontSize);
    c->setColor(state_.strokeColor);

    float boxX, boxW;
    if (just == visage::Font::kCenter) {
        boxX = 0.0f; boxW = 2.0f * drawX;
    } else if (just == visage::Font::kRight) {
        boxX = 0.0f; boxW = drawX;
    } else {
        boxX = drawX; boxW = static_cast<float>(c->width()) - drawX;
    }

    c->text(visage::String(text), vFont, just, boxX, drawY, boxW, lineH);
}

float VisageRenderer::measureText(void* canvas, const std::string& text)
{
    if (text.empty()) return 0.0f;
    visage::Font vFont = makeFont(state_.fontSize);
    std::u32string u32 = visage::String::convertUtf8ToUtf32<std::u32string>(text);
    return vFont.stringWidth(u32.c_str(), static_cast<int>(u32.size()));
}

void VisageRenderer::setComponentMouseDownCallback(std::function<void(void*, float, float)> cb)
{
    componentMouseDownCallback_ = std::move(cb);
}

void VisageRenderer::setComponentMouseUpCallback(std::function<void(void*, float, float)> cb)
{
    componentMouseUpCallback_ = std::move(cb);
}

void VisageRenderer::setComponentMouseDragCallback(std::function<void(void*, float, float)> cb)
{
    componentMouseDragCallback_ = std::move(cb);
}

void VisageRenderer::setComponentMouseEnterCallback(std::function<void(void*)> cb)
{
    componentMouseEnterCallback_ = std::move(cb);
}

void VisageRenderer::setComponentMouseExitCallback(std::function<void(void*)> cb)
{
    componentMouseExitCallback_ = std::move(cb);
}

void VisageRenderer::setComponentMouseWheelCallback(std::function<void(void*, float, float)> cb)
{
    componentMouseWheelCallback_ = std::move(cb);
}

void VisageRenderer::redraw(void *component)
{
    static_cast<visage::Frame*>(component)->redraw();
}

void VisageRenderer::redrawAll()
{
    rootFrame_->redrawAll();
}

double VisageRenderer::getTime(void *canvas)
{
    return static_cast<visage::Canvas*>(canvas)->time();
}

void VisageRenderer::setPostEffectForComponent(void* component, const PostEffectSpec& spec) {
    if (spec.type == "bloom") {
        auto* bloom = new visage::BloomPostEffect();
        bloom->setBloomSize(spec.size);
        bloom->setBloomIntensity(spec.intensity);
        static_cast<visage::Frame*>(component)->setPostEffect(bloom);
    } else if (spec.type == "blur") {
        auto* blur = new visage::BlurPostEffect();
        blur->setBlurRadius(spec.size > 0.0f ? spec.size : 20.0f);
        static_cast<visage::Frame*>(component)->setPostEffect(blur);
    }
}

void VisageRenderer::beginLayer(void* canvas, float opacity)
{
    auto* c = static_cast<visage::Canvas*>(canvas);
    if (!c) return;
    int depth = static_cast<int>(layerStack_.size());
    if (depth >= static_cast<int>(layerPool_.size())) {
        auto reg = std::make_unique<visage::Region>();
        c->addRegion(reg.get());
        int w = c->width(), h = c->height();
        reg->setBounds(0, 0, w, h);
        reg->setNeedsLayer(true);
        layerPool_.push_back(std::move(reg));
    }
    layerStack_.push_back({ opacity });
    c->beginRegion(layerPool_[depth].get());
}

void VisageRenderer::endLayer(void* canvas)
{
    auto* c = static_cast<visage::Canvas*>(canvas);
    if (!c || layerStack_.empty()) return;
    float opacity = layerStack_.back().opacity;
    layerStack_.pop_back();
    if (opacity < 1.0f) {
        c->setBlendMode(visage::BlendMode::Mult);
        c->setColor(visage::Color(0xffffffff).withAlpha(opacity));
        c->fill(0, 0, static_cast<float>(c->width()), static_cast<float>(c->height()));
    }
    c->endRegion();
    c->setBlendMode(state_.blendMode);
}
void VisageRenderer::setHdrMultiplier(void* canvas, float mult)
{
    state_.hdrMultiplier = std::max(0.0f, mult);
}
