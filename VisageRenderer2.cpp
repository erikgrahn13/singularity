#include <memory>
#include "VisageRenderer2.h"
#include <visage_graphics/tests/lato_regular.h>
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

VisageRenderer::VisageRenderer(void *parentHandle)
{
    rootFrame_ = static_cast<visage::ApplicationWindow*>(parentHandle);
    rootFrame_->onDraw() += [this](visage::Canvas& canvas) {
        canvas.setColor(0xff000000);
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
    parentFrame->addChild(std::move(child));

    std::cout << "VisageRenderer created child: " << childPtr
              << " parent: " << parentFrame << std::endl;

    return childPtr;
}

void VisageRenderer::setBounds(void *component, float x, float y, float w, float h)
{
    auto* frame = static_cast<visage::Frame*>(component);
    frame->setBounds(x, y, w, h);

    std::cout << "VisageRenderer set bounds on: " << frame
              << " -> " << x << ", " << y << ", " << w << ", " << h << std::endl;
}

void VisageRenderer::setDrawCallback(void *component, std::function<void(void *canvas)> cb)
{
    auto* frame = static_cast<visage::Frame*>(component);

    frame->onDraw() = [cb, frame](visage::Canvas& canvas) {
        std::cout << "onDraw fired for frame: " << frame << std::endl;
        cb(&canvas);
    };
}

void VisageRenderer::clear()
{
    while (!rootFrame_->children().empty())
        rootFrame_->removeChild(rootFrame_->children().back());
    rootFrame_->redraw();
    std::cout << "Renderer cleared" << std::endl;
}

void VisageRenderer::fillRect(void *canvas, float x, float y, float w, float h)
{
    auto* c = static_cast<visage::Canvas*>(canvas);
    c->setColor(state_.fillColor);
    c->fill(x + state_.translateX, y + state_.translateY, w, h);
}

void VisageRenderer::strokeRect(void *canvas, float x, float y, float w, float h)
{
    auto* c = static_cast<visage::Canvas*>(canvas);
    c->setColor(state_.strokeColor);
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
}

void VisageRenderer::setStrokeStyle(void* canvas, const std::string& color)
{
    state_.strokeColor = parseColorString(color);
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

void VisageRenderer::ellipse(void* canvas, float cx, float cy, float rx, float ry,
                              float /*rotation*/, float startAngle, float endAngle,
                              bool anticlockwise)
{
    float tx = state_.translateX, ty = state_.translateY;

    if (std::abs(endAngle - startAngle) >= 2.0f * kPi - 0.001f) {
        currentPath_.addEllipse(cx + tx, cy + ty, rx, ry);
        return;
    }

    float sweep = anticlockwise ? (startAngle - endAngle) : (endAngle - startAngle);
    while (sweep < 0) sweep += 2.0f * kPi;
    while (sweep > 2.0f * kPi) sweep -= 2.0f * kPi;

    int segments = std::max(1, static_cast<int>(sweep / (kPi / 2.0f)) + 1);
    float segAngle = (anticlockwise ? -sweep : sweep) / segments;

    float sx = (cx + tx) + rx * std::cos(startAngle);
    float sy = (cy + ty) + ry * std::sin(startAngle);

    if (currentPath_.numPoints() == 0)
        currentPath_.moveTo(sx, sy);
    else
        currentPath_.lineTo(sx, sy);

    float alpha2 = std::tan(segAngle / 4.0f);
    float alphaFactor = std::sin(segAngle) * (std::sqrt(4.0f + 3.0f * alpha2 * alpha2) - 1.0f) / 3.0f;

    float angle = startAngle;
    for (int i = 0; i < segments; ++i) {
        float ex_ = (cx + tx) + rx * std::cos(angle + segAngle);
        float ey_ = (cy + ty) + ry * std::sin(angle + segAngle);
        float dx1 = (cx + tx) + rx * std::cos(angle);
        float dy1 = (cy + ty) + ry * std::sin(angle);
        currentPath_.bezierTo(
            dx1 - alphaFactor * rx * std::sin(angle),   dy1 + alphaFactor * ry * std::cos(angle),
            ex_ + alphaFactor * rx * std::sin(angle + segAngle), ey_ - alphaFactor * ry * std::cos(angle + segAngle),
            ex_, ey_);
        angle += segAngle;
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
    c->setColor(state_.fillColor);
    c->fill(currentPath_);
}

void VisageRenderer::stroke(void* canvas)
{
    if (currentPath_.numPoints() == 0) return;
    auto* c = static_cast<visage::Canvas*>(canvas);
    c->setColor(state_.strokeColor);
    // Canvas::stroke() calls a non-const method on Path internally; build the
    // stroked path ourselves and fill it instead.
    visage::Path stroked = currentPath_.stroke(state_.lineWidth);
    c->fill(stroked);
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
    c->setColor(state_.fillColor);

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
