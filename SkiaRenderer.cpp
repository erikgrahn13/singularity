#include "tmpshader.h"
#include "SkiaRenderer.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"

#ifdef _WIN32
#  include "include/ports/SkTypeface_win.h"
#elif __APPLE__
#  include "include/ports/SkFontMgr_mac_ct.h"
#else
#  include "include/ports/SkFontMgr_fontconfig.h"
#endif

SkiaRenderer::SkiaRenderer(int width, int height)
{
    skiaSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));

    const char* fontFamily;
    SkFontStyle fonStyle;

#ifdef _WIN32
        fontMgr = SkFontMgr_New_DirectWrite();
#elif __APPLE__
        fontMgr = SkFontMgr_New_CoreText(nullptr);
#else
        fontMgr = SkFontMgr_New_FontConfig(nullptr);
#endif
    defaultTypeface = fontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
}

DrawingContent SkiaRenderer::getDrawingContent()
{
    SkPixmap pixmap;
    skiaSurface->peekPixels(&pixmap);
    return {pixmap.addr(), pixmap.rowBytes(), skiaSurface->width(), skiaSurface->height()};
}

SkColor SkiaRenderer::applyAlpha(SkColor color) const
{
    uint8_t a = (uint8_t)(SkColorGetA(color) * currentState.globalAlpha);
    return SkColorSetA(color, a);
}

void SkiaRenderer::clear()
{
    skiaSurface->getCanvas()->clear(SK_ColorBLACK);
    currentState = DrawState{};
    stateStack.clear();
}

void SkiaRenderer::save()
{
    stateStack.push_back(currentState);
    skiaSurface->getCanvas()->save();
}

void SkiaRenderer::restore()
{
    if(stateStack.empty())
        return;
    currentState = stateStack.back();
    stateStack.pop_back();
    skiaSurface->getCanvas()->restore();
}

void SkiaRenderer::setGlobalAlpha(float alpha)
{
    currentState.globalAlpha = alpha;
}

// TODO: implement all the names colors, and also HSLA and RGBA
void SkiaRenderer::setFillStyle(const std::string& color)
{
    if (color.empty()) 
    {
        currentState.fillStyle = SK_ColorBLACK;
        return;
    }

    if (color[0] == '#') {
        std::string hex = color.substr(1);
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() == 6) {
            unsigned int rgb = (unsigned int)std::stoul(hex, nullptr, 16);
            currentState.fillStyle = SkColorSetARGB(0xFF, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            return;
        } else if (hex.size() == 8) {
            unsigned long rgba = std::stoul(hex, nullptr, 16);
            currentState.fillStyle = SkColorSetARGB(rgba & 0xFF, (rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF);
            return;
        }
        currentState.fillStyle = SK_ColorBLACK;
    }
}

void SkiaRenderer::setStrokeStyle(const std::string &color)
{
    if (color.empty()) 
    {
        currentState.strokeStyle = SK_ColorBLACK;
        return;
    }

    if (color[0] == '#') {
        std::string hex = color.substr(1);
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() == 6) {
            unsigned int rgb = (unsigned int)std::stoul(hex, nullptr, 16);
            currentState.strokeStyle = SkColorSetARGB(0xFF, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            return;
        } else if (hex.size() == 8) {
            unsigned long rgba = std::stoul(hex, nullptr, 16);
            currentState.strokeStyle = SkColorSetARGB(rgba & 0xFF, (rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF);
            return;
        }
        currentState.strokeStyle = SK_ColorBLACK;
    }
}

void SkiaRenderer::setLineWidth(float lineWidth)
{
    currentState.lineWidth = lineWidth;
}

void SkiaRenderer::setLineCap(const std::string &cap)
{
    if(cap == "butt")
    {
        currentState.lineCap = SkPaint::kButt_Cap;
    }
    else if(cap == "round")
    {
        currentState.lineCap = SkPaint::kRound_Cap;
    }
    else if(cap == "square")
    {
        currentState.lineCap = SkPaint::kSquare_Cap;
    }
}

void SkiaRenderer::setLineJoin(const std::string &join)
{
    if(join == "miter")
    {
        currentState.lineJoin = SkPaint::kMiter_Join;
    }
    else if(join == "round")
    {
        currentState.lineJoin = SkPaint::kRound_Join;
    }
    else if(join == "bevel")
    {
        currentState.lineJoin = SkPaint::kBevel_Join;
    }
}

void SkiaRenderer::fillRect(float x, float y, float width, float height)
{
    SkPaint paint;
    paint.setColor(applyAlpha(currentState.fillStyle));
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true); 
    skiaSurface->getCanvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}

void SkiaRenderer::strokeRect(float x, float y, float width, float height)
{
    SkPaint paint;
    paint.setColor(applyAlpha(currentState.strokeStyle));
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(currentState.lineWidth);
    paint.setStrokeCap(currentState.lineCap);
    paint.setStrokeJoin(currentState.lineJoin);
    paint.setAntiAlias(true); 
    skiaSurface->getCanvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}

void SkiaRenderer::roundRect(float x, float y, float width, float height, float radii)
{
    SkRRect roundRect;
    roundRect.setRectXY(SkRect::MakeXYWH(x, y, width, height), radii, radii);
    currentPath.addRRect(roundRect);
}

void SkiaRenderer::beginPath()
{
    currentPath = SkPathBuilder();
}

void SkiaRenderer::stroke()
{
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(applyAlpha(currentState.strokeStyle));
    paint.setStrokeWidth(currentState.lineWidth);
    paint.setStrokeCap(currentState.lineCap);
    paint.setStrokeJoin(currentState.lineJoin);
    paint.setAntiAlias(true); 
    skiaSurface->getCanvas()->drawPath(currentPath.snapshot(), paint);
}

void SkiaRenderer::fill()
{
    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(applyAlpha(currentState.fillStyle));
    paint.setAntiAlias(true);
    skiaSurface->getCanvas()->drawPath(currentPath.snapshot(), paint);
}

void SkiaRenderer::moveTo(float x, float y)
{
    currentPath.moveTo(x, y);
}

void SkiaRenderer::lineTo(float x, float y)
{
    currentPath.lineTo(x, y);
}

void SkiaRenderer::closePath()
{
    currentPath.close();
}

void SkiaRenderer::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    currentPath.quadTo(cpx, cpy, x, y);
}

void SkiaRenderer::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y)
{
    currentPath.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void SkiaRenderer::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    currentPath.arcTo(SkPoint::Make(x1, y1), SkPoint::Make(x2, y2), radius);
}

void SkiaRenderer::ellipse(float x, float y, float radiusX, float radiusY, float rotation, float startAngle, float endAngle)
{
    // Build arc centered at origin (no rotation yet)
    SkRect oval = SkRect::MakeXYWH(-radiusX, -radiusY, radiusX * 2, radiusY * 2);
    float startDeg = startAngle * (180.f / SK_FloatPI);
    float sweepDeg = (endAngle - startAngle) * (180.f / SK_FloatPI);

    SkPathBuilder tmp;
    tmp.addArc(oval, startDeg, sweepDeg);

    // Rotate around origin, then translate to (x, y)
    SkMatrix matrix;
    matrix.setRotate(rotation * (180.f / SK_FloatPI));
    matrix.postTranslate(x, y);

    currentPath.addPath(tmp.snapshot(), matrix);
}

void SkiaRenderer::rect(float x, float y, float width, float height)
{
    currentPath.addRect(SkRect::MakeXYWH(x, y, width, height));
}

void SkiaRenderer::arc(float x, float y, float radius, float startAngle, float endAngle)
{
    SkRect oval = SkRect::MakeXYWH(x - radius, y - radius, radius * 2, radius *2);
    float sweepDeg = (endAngle - startAngle) * (180.f / SK_FloatPI);
    currentPath.addArc(oval, startAngle * (180.f / SK_FloatPI), sweepDeg);
}

void SkiaRenderer::fillText(const std::string &text, float x, float y)
{
    SkFont font(defaultTypeface, currentState.fontSize);

    float width = font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);

    if(currentState.textAlign == "center")
    {
        x -= width / 2.f;
    }
    else if(currentState.textAlign == "right" || currentState.textAlign == "end")
    {
        x -= width;
    }


    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(applyAlpha(currentState.fillStyle));
    paint.setAntiAlias(true);
    skiaSurface->getCanvas()->drawString(text.c_str(), x, y, font, paint);
}

void SkiaRenderer::strokeText(const std::string &text, float x, float y)
{
    SkFont font(defaultTypeface, currentState.fontSize);
    float width = font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);

    if(currentState.textAlign == "center")
    {
        x -= width / 2.f;
    }
    else if(currentState.textAlign == "right" || currentState.textAlign == "end")
    {
        x -= width;
    }

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(applyAlpha(currentState.strokeStyle));
    paint.setAntiAlias(true);
    skiaSurface->getCanvas()->drawString(text.c_str(), x, y, font, paint);
}

float SkiaRenderer::measureText(const std::string &text)
{
    SkFont font(defaultTypeface, currentState.fontSize);
    return font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);
}

void SkiaRenderer::font(const std::string &text)
{
    auto pxPos = text.find("px");
    if (pxPos == std::string::npos) return; // not a valid font string

    size_t sizeStart = pxPos;
    while (sizeStart > 0 && (std::isdigit((unsigned char)text[sizeStart - 1]) || text[sizeStart - 1] == '.'))
        --sizeStart;

    // 3. Extract the size number e.g. "16"
    currentState.fontSize = std::stof(text.substr(sizeStart, pxPos - sizeStart));

    // 4. Everything after "px " is the font family e.g. "Arial"
    currentState.fontFamily = (pxPos + 3 <= text.size()) ? text.substr(pxPos + 3) : "";

    // 5. Check for "bold" / "italic" in the prefix
    std::string prefix = text.substr(0, sizeStart);
    bool bold   = prefix.find("bold")   != std::string::npos;
    bool italic = prefix.find("italic") != std::string::npos;
    currentState.fontStyle = SkFontStyle(
        bold   ? SkFontStyle::kBold_Weight  : SkFontStyle::kNormal_Weight,
        SkFontStyle::kNormal_Width,
        italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant
    );

    defaultTypeface = fontMgr->legacyMakeTypeface(currentState.fontFamily.c_str(), currentState.fontStyle);
}

void SkiaRenderer::textAlign(const std::string &align)
{
    currentState.textAlign = align;
}

int SkiaRenderer::getWidth() const  { return skiaSurface->width(); }
int SkiaRenderer::getHeight() const { return skiaSurface->height(); }

std::unique_ptr<IRenderer> createRenderer(int width, int height)
{
    return std::make_unique<SkiaRenderer>(width, height);
}

void SkiaRenderer::renderBackground(float t)
{
    SkCanvas* canvas = skiaSurface->getCanvas();
    DrawMetaballsBackground(canvas, skiaSurface->width(), skiaSurface->height(), t);
}