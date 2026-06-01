#include "SkiaRenderer2.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"

#include <cstdlib>
#include <stdexcept>

// Parse CSS color strings like "#rrggbb" / "#rrggbbaa" / "#rgb" into SkColor.
static SkColor parseCSSColor(const std::string& color, float globalAlpha = 1.0f)
{
    auto applyAlpha = [&](SkColor c) -> SkColor {
        uint8_t a = static_cast<uint8_t>(SkColorGetA(c) * globalAlpha);
        return SkColorSetA(c, a);
    };

    if (!color.empty() && color[0] == '#') {
        unsigned long hex = std::strtoul(color.c_str() + 1, nullptr, 16);
        size_t len = color.size() - 1;
        if (len == 6) {
            uint8_t r = (hex >> 16) & 0xFF;
            uint8_t g = (hex >>  8) & 0xFF;
            uint8_t b = (hex >>  0) & 0xFF;
            return applyAlpha(SkColorSetARGB(255, r, g, b));
        } else if (len == 8) {
            uint8_t r = (hex >> 24) & 0xFF;
            uint8_t g = (hex >> 16) & 0xFF;
            uint8_t b = (hex >>  8) & 0xFF;
            uint8_t a = (hex >>  0) & 0xFF;
            return applyAlpha(SkColorSetARGB(a, r, g, b));
        } else if (len == 3) {
            uint8_t r = ((hex >> 8) & 0xF) * 17;
            uint8_t g = ((hex >> 4) & 0xF) * 17;
            uint8_t b = ((hex >> 0) & 0xF) * 17;
            return applyAlpha(SkColorSetARGB(255, r, g, b));
        }
    }
    // Fallback: white
    return applyAlpha(SK_ColorWHITE);
}

void SkiaRenderer::resize(int w, int h) {
    IRenderer::resize(w, h); // stores width_/height_
    // TODO: reallocate Skia surface at new size
}

void* SkiaRenderer::currentCanvas() const { return currentCanvas_; }

void  SkiaRenderer::fillRect(float x, float y, float width, float height)
{
    if (!currentCanvas_) return;
    SkPaint paint;
    paint.setColor(fillColor_);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true);
    currentCanvas_->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}
void  SkiaRenderer::strokeRect(float, float, float, float) {}
void  SkiaRenderer::clearRect(float, float, float, float) {}

void  SkiaRenderer::beginPath(void*) {}
void  SkiaRenderer::moveTo(float, float) {}
void  SkiaRenderer::lineTo(float, float) {}
void  SkiaRenderer::closePath(void*) {}
void  SkiaRenderer::arc(float, float, float, float, float, bool) {}
void  SkiaRenderer::arcTo(float, float, float, float, float) {}
void  SkiaRenderer::quadraticCurveTo(float, float, float, float) {}
void  SkiaRenderer::bezierCurveTo(float, float, float, float, float, float) {}
void  SkiaRenderer::ellipse(float, float, float, float, float, float, float, bool) {}
void  SkiaRenderer::rect(float, float, float, float) {}
void  SkiaRenderer::roundRect(float, float, float, float, float) {}

void  SkiaRenderer::fill(void*) {}
void  SkiaRenderer::stroke(void*) {}

void  SkiaRenderer::fillText(const std::string&, float, float) {}
void  SkiaRenderer::strokeText(const std::string&, float, float) {}
float SkiaRenderer::measureText(const std::string&) { return 0.0f; }

void  SkiaRenderer::setFillStyle(const std::string& color)   { fillColor_   = parseCSSColor(color, globalAlpha_); }
void  SkiaRenderer::setStrokeStyle(const std::string& color) { strokeColor_ = parseCSSColor(color, globalAlpha_); }
void  SkiaRenderer::setLineWidth(float) {}
void  SkiaRenderer::setLineCap(const std::string&) {}
void  SkiaRenderer::setFont(const std::string&) {}
void  SkiaRenderer::setGlobalAlpha(float) {}
void  SkiaRenderer::setTextAlign(const std::string&) {}
void  SkiaRenderer::setTextBaseline(const std::string&) {}

int   SkiaRenderer::createLinearGradient(float, float, float, float) { return 0; }
int   SkiaRenderer::createRadialGradient(float, float, float, float, float, float) { return 0; }
void  SkiaRenderer::addColorStop(int, float, const std::string&, float) {}
void  SkiaRenderer::setFillStyleGradient(int) {}
void  SkiaRenderer::setStrokeStyleGradient(int) {}

void  SkiaRenderer::save(void*) {}
void  SkiaRenderer::restore(void*) {}

void  SkiaRenderer::translate(float, float) {}
void  SkiaRenderer::rotate(float) {}
void  SkiaRenderer::scale(float, float) {}
void  SkiaRenderer::resetTransform(void*) {}
