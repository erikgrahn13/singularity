#include "SkiaRenderer2.h"
#include "platform/IWindow.h"
#include "dawn/native/DawnNative.h"
#include "dawn/dawn_proc.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/Recording.h"
#include "include/gpu/graphite/GraphiteTypes.h"
#include "include/gpu/graphite/dawn/DawnGraphiteTypes.h"
#include "include/gpu/graphite/dawn/DawnBackendContext.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPath.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/utils/SkTextUtils.h"
#include "include/effects/SkGradientShader.h"
#include "include/gpu/graphite/Surface.h"


#include <algorithm>
#include <stdexcept>

#ifdef __linux__
#include "platform/linux/X11Window.h"
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
    #undef Always
    #undef Success
    #undef None
    #undef Status
    #undef Bool
    #undef True
    #undef False
#endif
// ── Helpers ───────────────────────────────────────────────────────────────────

static SkColor parseColor(const std::string& s, float alpha = 1.0f) {
    auto tinted = [&](SkColor c) {
        return SkColorSetA(c, (uint8_t)(SkColorGetA(c) * alpha));
    };
    if (s.size() > 1 && s[0] == '#') {
        unsigned long v = std::strtoul(s.c_str() + 1, nullptr, 16);
        size_t n = s.size() - 1;
        if (n == 6) return tinted(SkColorSetARGB(255, (v>>16)&0xFF, (v>>8)&0xFF, v&0xFF));
        if (n == 8) return tinted(SkColorSetARGB((v>>24)&0xFF,(v>>16)&0xFF,(v>>8)&0xFF,v&0xFF));
        if (n == 3) return tinted(SkColorSetARGB(255,((v>>8)&0xF)*17,((v>>4)&0xF)*17,(v&0xF)*17));
    }
    return tinted(SK_ColorWHITE);
}

SkPaint SkiaRenderer::fillPaint() const {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setAntiAlias(true);
    if (state_.fillGrad >= 0 && state_.fillGrad < (int)grads_.size()) {
        auto& g = grads_[state_.fillGrad];
        std::vector<SkColor>  colors;
        std::vector<SkScalar> pos;
        for (auto& [t, c] : g.stops) { pos.push_back(t); colors.push_back(c); }
        int n = (int)colors.size();
        if (g.type == Gradient::Type::Linear) {
            SkPoint pts[2] = { {g.x0, g.y0}, {g.x1, g.y1} };
            p.setShader(SkGradientShader::MakeLinear(pts, colors.data(), pos.data(), n, SkTileMode::kClamp));
        } else {
            p.setShader(SkGradientShader::MakeTwoPointConical(
                {g.x0, g.y0}, g.r0, {g.x1, g.y1}, g.r1,
                colors.data(), pos.data(), n, SkTileMode::kClamp));
        }
    } else {
        p.setColor(SkColorSetA(state_.fillColor,
                               (uint8_t)(SkColorGetA(state_.fillColor) * state_.alpha)));
    }
    return p;
}

SkPaint SkiaRenderer::strokePaint() const {
    SkPaint p;
    p.setStyle(SkPaint::kStroke_Style);
    p.setAntiAlias(true);
    p.setColor(SkColorSetA(state_.strokeColor,
                           (uint8_t)(SkColorGetA(state_.strokeColor) * state_.alpha)));
    p.setStrokeWidth(state_.lineWidth);
    p.setStrokeCap(state_.lineCap);
    p.setStrokeJoin(state_.lineJoin);
    return p;
}

// ── Construction ─────────────────────────────────────────────────────────────

SkiaRenderer::SkiaRenderer(std::string_view resourcePath) {
    dawnProcSetProcs(&dawn::native::GetProcs());

    wgpu::InstanceDescriptor instanceDesc{};
    wgpu::InstanceFeatureName requiredFeatures[] = { wgpu::InstanceFeatureName::TimedWaitAny };
    instanceDesc.requiredFeatureCount = 1;
    instanceDesc.requiredFeatures = requiredFeatures;
    instance_ = wgpu::CreateInstance(&instanceDesc);

    wgpu::Future f1 = instance_.RequestAdapter(nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView) {
            if (status == wgpu::RequestAdapterStatus::Success)
                adapter_ = std::move(a);
            else
                throw std::runtime_error("Dawn: RequestAdapter failed");
        });
    instance_.WaitAny(f1, UINT64_MAX);

    wgpu::Future f2 = adapter_.RequestDevice(nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView) {
            if (status == wgpu::RequestDeviceStatus::Success)
                device_ = std::move(d);
            else
                throw std::runtime_error("Dawn: RequestDevice failed");
        });
    instance_.WaitAny(f2, UINT64_MAX);


#ifdef __linux__
    fontMgr_  = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#elif __APPLE__
    fontMgr_ = SkFontMgr_New_CoreText(nullptr);
#endif
    typeface_ = fontMgr_->legacyMakeTypeface(nullptr, SkFontStyle());
}

std::unique_ptr<IRenderer> IRenderer::createRenderer(std::string_view r) {
    return std::make_unique<SkiaRenderer>(r);
}

// ── Swapchain ─────────────────────────────────────────────────────────────────

// ── Window attachment ─────────────────────────────────────────────────────────

void SkiaRenderer::attachToWindow(IWindow& window) {

#ifdef __linux__
    auto& nativeWindow = static_cast<X11Window&>(window);

    wgpu::SurfaceSourceXlibWindow libDesc{};
    libDesc.display = nativeWindow.display();
    libDesc.window  = static_cast<uint64_t>(nativeWindow.xwindow());

#endif

    wgpu::SurfaceDescriptor surfDesc{};
    surfDesc.nextInChain = &libDesc;
    surface_ = instance_.CreateSurface(&surfDesc);

    wgpu::SurfaceConfiguration config{};
    config.device = device_;
    config.format = wgpu::TextureFormat::BGRA8Unorm; // or query preferred
    config.width  = window.width();
    config.height = window.height();
    config.presentMode = wgpu::PresentMode::Fifo;
    surface_.Configure(&config);

    skgpu::graphite::DawnBackendContext dawnCtx{};
    dawnCtx.fInstance = instance_;
    dawnCtx.fDevice   = device_;
    dawnCtx.fQueue    = device_.GetQueue();
    ctx_ = skgpu::graphite::ContextFactory::MakeDawn(dawnCtx, {});
    rec_ = ctx_->makeRecorder();
}

// ── Destructor ────────────────────────────────────────────────────────────────

SkiaRenderer::~SkiaRenderer() {

}

// ── Frame lifecycle ───────────────────────────────────────────────────────────

void SkiaRenderer::resize(int w, int h) {
    IRenderer::resize(w, h);
    if (!surface_) return;

    wgpu::SurfaceConfiguration config{};
    config.device = device_;
    config.format = wgpu::TextureFormat::BGRA8Unorm;
    config.width  = w;
    config.height = h;
    config.presentMode = wgpu::PresentMode::Fifo;
    surface_.Configure(&config);
}

void* SkiaRenderer::beginFrame() {
    wgpu::SurfaceTexture surfTex;
    surface_.GetCurrentTexture(&surfTex);
    if (surfTex.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
        surfTex.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal)
        return nullptr;

    auto backendTex = skgpu::graphite::BackendTextures::MakeDawn(surfTex.texture.Get());

    skSurface_ = SkSurfaces::WrapBackendTexture(rec_.get(),
                                                 backendTex,
                                                 kBGRA_8888_SkColorType,
                                                 nullptr, nullptr);
    return skSurface_->getCanvas();
}

SkCanvas* SkiaRenderer::canvas() const {
    return skSurface_ ? skSurface_->getCanvas() : nullptr;
}

void* SkiaRenderer::currentCanvas() const { return canvas(); }

void SkiaRenderer::present() {
    auto recording = rec_->snap();
    if (!recording) return;

    skgpu::graphite::InsertRecordingInfo info{};
    info.fRecording     = recording.get();
    info.fTargetSurface = skSurface_.get();

    ctx_->insertRecording(info);
    ctx_->submit(skgpu::graphite::SyncToCpu::kNo);

    surface_.Present();
    skSurface_.reset();
}

// ── Draw methods ─────────────────────────────────────────────────────────────

void SkiaRenderer::fillRect(float x, float y, float w, float h) {
    if (auto* c = canvas()) c->drawRect(SkRect::MakeXYWH(x,y,w,h), fillPaint());
}
void SkiaRenderer::strokeRect(float x, float y, float w, float h) {
    if (auto* c = canvas()) c->drawRect(SkRect::MakeXYWH(x,y,w,h), strokePaint());
}
void SkiaRenderer::clearRect(float x, float y, float w, float h) {
    if (auto* c = canvas()) {
        c->save();
        c->clipRect(SkRect::MakeXYWH(x,y,w,h));
        c->drawColor(SK_ColorTRANSPARENT, SkBlendMode::kClear);
        c->restore();
    }
}

void SkiaRenderer::beginPath(void*)          { path_ = SkPathBuilder(); }
void SkiaRenderer::moveTo(float x, float y)  { path_.moveTo(x, y); }
void SkiaRenderer::lineTo(float x, float y)  { path_.lineTo(x, y); }
void SkiaRenderer::closePath(void*)          { path_.close(); }

void SkiaRenderer::arc(float cx, float cy, float r, float start, float end, bool ccw) {
    SkRect oval = SkRect::MakeXYWH(cx-r, cy-r, r*2, r*2);
    float sweep = (end - start) * (180.f / SK_FloatPI);
    if (ccw) sweep -= 360.f;
    path_.addArc(oval, start * (180.f / SK_FloatPI), sweep);
}
void SkiaRenderer::arcTo(float x1, float y1, float x2, float y2, float r) {
    path_.arcTo(SkPoint::Make(x1,y1), SkPoint::Make(x2,y2), r);
}
void SkiaRenderer::quadraticCurveTo(float cpx, float cpy, float x, float y) {
    path_.quadTo(cpx, cpy, x, y);
}
void SkiaRenderer::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) {
    path_.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
}
void SkiaRenderer::ellipse(float cx, float cy, float rx, float ry,
                            float rot, float start, float end, bool ccw) {
    float sweep = (end - start) * (180.f / SK_FloatPI);
    if (ccw) sweep -= 360.f;
    SkPathBuilder tmp;
    tmp.addArc(SkRect::MakeXYWH(-rx,-ry,rx*2,ry*2), start*(180.f/SK_FloatPI), sweep);
    SkMatrix m;
    m.setRotate(rot * (180.f / SK_FloatPI));
    m.postTranslate(cx, cy);
    path_.addPath(tmp.snapshot(), m);
}
void SkiaRenderer::rect(float x, float y, float w, float h) {
    path_.addRect(SkRect::MakeXYWH(x,y,w,h));
}
void SkiaRenderer::roundRect(float x, float y, float w, float h, float r) {
    SkRRect rr;
    rr.setRectXY(SkRect::MakeXYWH(x,y,w,h), r, r);
    path_.addRRect(rr);
}

void SkiaRenderer::fill(void*)   { if (auto* c = canvas()) c->drawPath(path_.snapshot(), fillPaint()); }
void SkiaRenderer::stroke(void*) { if (auto* c = canvas()) c->drawPath(path_.snapshot(), strokePaint()); }

void SkiaRenderer::fillText(const std::string& text, float x, float y) {
    if (auto* c = canvas()) {
        SkFont font(typeface_, state_.fontSize);
        SkTextUtils::Align a = SkTextUtils::kLeft_Align;
        if (state_.textAlign == "center")                       a = SkTextUtils::kCenter_Align;
        else if (state_.textAlign == "right" ||
                 state_.textAlign == "end")                     a = SkTextUtils::kRight_Align;
        SkTextUtils::DrawString(c, text.c_str(), x, y, font, fillPaint(), a);
    }
}
void SkiaRenderer::strokeText(const std::string& text, float x, float y) {
    if (auto* c = canvas())
        SkTextUtils::DrawString(c, text.c_str(), x, y, SkFont(typeface_, state_.fontSize), strokePaint());
}
float SkiaRenderer::measureText(const std::string& text) {
    return SkFont(typeface_, state_.fontSize)
        .measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);
}

// ── Style setters ─────────────────────────────────────────────────────────────

void SkiaRenderer::setFillStyle(const std::string& c)   { state_.fillColor = parseColor(c); state_.fillGrad = -1; }
void SkiaRenderer::setStrokeStyle(const std::string& c) { state_.strokeColor = parseColor(c); }
void SkiaRenderer::setLineWidth(float w)                { state_.lineWidth = w; }
void SkiaRenderer::setLineCap(const std::string& cap) {
    if      (cap == "round")  state_.lineCap = SkPaint::kRound_Cap;
    else if (cap == "square") state_.lineCap = SkPaint::kSquare_Cap;
    else                      state_.lineCap = SkPaint::kButt_Cap;
}
void SkiaRenderer::setFont(const std::string& font) {
    auto px = font.find("px");
    if (px != std::string::npos)
        try { state_.fontSize = std::stof(font.substr(0, px)); } catch (...) {}
}
void SkiaRenderer::setGlobalAlpha(float a)               { state_.alpha = a; }
void SkiaRenderer::setTextAlign(const std::string& a)    { state_.textAlign = a; }
void SkiaRenderer::setTextBaseline(const std::string& b) { state_.textBase = b; }

// ── Gradients ────────────────────────────────────────────────────────────────

int SkiaRenderer::createLinearGradient(float x0, float y0, float x1, float y1) {
    grads_.push_back({ Gradient::Type::Linear, x0, y0, x1, y1, 0, 0, {} });
    return (int)grads_.size() - 1;
}
int SkiaRenderer::createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) {
    grads_.push_back({ Gradient::Type::Radial, x0, y0, x1, y1, r0, r1, {} });
    return (int)grads_.size() - 1;
}
void SkiaRenderer::addColorStop(int id, float offset, const std::string& color, float) {
    if (id >= 0 && id < (int)grads_.size())
        grads_[id].stops.push_back({ offset, parseColor(color) });
}
void SkiaRenderer::setFillStyleGradient(int id)   { state_.fillGrad = id; }
void SkiaRenderer::setStrokeStyleGradient(int)    {}

// ── State ─────────────────────────────────────────────────────────────────────

void SkiaRenderer::save(void*) {
    if (auto* c = canvas()) { stack_.push_back(state_); c->save(); }
}
void SkiaRenderer::restore(void*) {
    if (auto* c = canvas(); c && !stack_.empty()) {
        state_ = stack_.back(); stack_.pop_back(); c->restore();
    }
}

// ── Transforms ───────────────────────────────────────────────────────────────

void SkiaRenderer::translate(float x, float y)  { if (auto* c = canvas()) c->translate(x, y); }
void SkiaRenderer::rotate(float a)               { if (auto* c = canvas()) c->rotate(a * (180.f / SK_FloatPI)); }
void SkiaRenderer::scale(float x, float y)       { if (auto* c = canvas()) c->scale(x, y); }
void SkiaRenderer::resetTransform(void*)         { if (auto* c = canvas()) c->resetMatrix(); }
