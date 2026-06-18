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
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/gpu/graphite/Image.h"
#include "include/effects/SkImageFilters.h"
#include "include/core/SkColorFilter.h"

#include <algorithm>
#include <filesystem>
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
#elif __APPLE__
#include "include/ports/SkFontMgr_mac_ct.h"
#include "platform/macos/AppKitWindow.h"
#endif
// ── Helpers ───────────────────────────────────────────────────────────────────

// Parses a CSS color string to SkColor4f (linear values for color(srgb-linear),
// normalized 0-1 for #hex / rgba() / rgb()). Values may exceed 1.0 for HDR.
static SkColor4f parseColor4f(const std::string& s, float alpha = 1.0f) {
    if (s.size() > 1 && s[0] == '#') {
        unsigned long v = std::strtoul(s.c_str() + 1, nullptr, 16);
        size_t n = s.size() - 1;
        if (n == 6) return { ((v>>16)&0xFF)/255.f, ((v>>8)&0xFF)/255.f, (v&0xFF)/255.f, alpha };
        if (n == 8) return { ((v>>16)&0xFF)/255.f, ((v>>8)&0xFF)/255.f, (v&0xFF)/255.f, ((v>>24)&0xFF)/255.f * alpha };
        if (n == 3) return { ((v>>8)&0xF)/15.f,    ((v>>4)&0xF)/15.f,   (v&0xF)/15.f,  alpha };
    }
    if (s.size() > 5 && s[0]=='r' && s[1]=='g' && s[2]=='b' && s[3]=='a' && s[4]=='(') {
        float r=0, g=0, b=0, a=1;
        sscanf(s.c_str(), "rgba(%f,%f,%f,%f)", &r, &g, &b, &a);
        return { r/255.f, g/255.f, b/255.f, a * alpha };
    }
    if (s.size() > 4 && s[0]=='r' && s[1]=='g' && s[2]=='b' && s[3]=='(') {
        float r=0, g=0, b=0;
        sscanf(s.c_str(), "rgb(%f,%f,%f)", &r, &g, &b);
        return { r/255.f, g/255.f, b/255.f, alpha };
    }
    // color(srgb-linear r g b [a]) — HDR linear values, can exceed 1.0
    if (s.size() > 18 && s.rfind("color(srgb-linear", 0) == 0) {
        float r=0, g=0, b=0, a=1;
        sscanf(s.c_str(), "color(srgb-linear %f %f %f %f)", &r, &g, &b, &a);
        return { r, g, b, a * alpha };
    }
    return {1.f, 1.f, 1.f, alpha};
}

void SkiaRenderer::applyShadow(SkPaint& p) const {
    if (state_.shadowBlur <= 0.0f && state_.shadowOffsetX == 0.0f && state_.shadowOffsetY == 0.0f)
        return;
    if (state_.shadowColor.fA <= 0.0f)
        return;
    // Convert to SkColor (clamped 8-bit) for the image filter
    SkColor shadowC = state_.shadowColor.toSkColor();
    p.setImageFilter(
        SkImageFilters::DropShadow(
            state_.shadowOffsetX, state_.shadowOffsetY,
            state_.shadowBlur * 0.5f, state_.shadowBlur * 0.5f,
            shadowC, nullptr));
}

SkPaint SkiaRenderer::fillPaint() const {
    SkPaint p;
    p.setStyle(SkPaint::kFill_Style);
    p.setAntiAlias(true);
    if (state_.fillGrad >= 0 && state_.fillGrad < (int)grads_.size()) {
        auto& g = grads_[state_.fillGrad];
        std::vector<SkColor4f> colors;
        std::vector<SkScalar>  pos;
        for (auto& [t, c] : g.stops) {
            pos.push_back(t);
            SkColor4f ca = c; ca.fA *= state_.alpha;
            colors.push_back(ca);
        }
        int n = (int)colors.size();
        auto cs = SkColorSpace::MakeSRGB();
        if (g.type == Gradient::Type::Linear) {
            SkPoint pts[2] = { {g.x0, g.y0}, {g.x1, g.y1} };
            p.setShader(SkGradientShader::MakeLinear(pts, colors.data(), cs, pos.data(), n, SkTileMode::kClamp));
        } else {
            p.setShader(SkGradientShader::MakeTwoPointConical(
                {g.x0, g.y0}, g.r0, {g.x1, g.y1}, g.r1,
                colors.data(), cs, pos.data(), n, SkTileMode::kClamp));
        }
    } else {
        SkColor4f c = state_.fillColor;
        c.fA *= state_.alpha;
        p.setColor4f(c, nullptr);
    }
    applyShadow(p);
    return p;
}

SkPaint SkiaRenderer::strokePaint() const {
    SkPaint p;
    p.setStyle(SkPaint::kStroke_Style);
    p.setAntiAlias(true);
    SkColor4f c = state_.strokeColor;
    c.fA *= state_.alpha;
    p.setColor4f(c, nullptr);
    p.setStrokeWidth(state_.lineWidth);
    p.setStrokeCap(state_.lineCap);
    p.setStrokeJoin(state_.lineJoin);
    applyShadow(p);
    return p;
}

// ── Construction ─────────────────────────────────────────────────────────────

SkiaRenderer::SkiaRenderer(std::string_view resourcePath)
    : resourcePath_(resourcePath)
{
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

    wgpu::SurfaceDescriptor surfDesc{};

#ifdef __linux__
    auto& nativeWindow = static_cast<X11Window&>(window);

    wgpu::SurfaceSourceXlibWindow libDesc{};
    libDesc.display = nativeWindow.display();
    libDesc.window  = static_cast<uint64_t>(nativeWindow.xwindow());
    surfDesc.nextInChain = &libDesc;
#elif __APPLE__
    wgpu::SurfaceSourceMetalLayer metalDesc{};
    metalDesc.layer = createMetalLayerForView(window.nativeHandle());
    surfDesc.nextInChain = &metalDesc;
#endif

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

    bloomStrength_ = 0.0f; // reset each frame — components must re-enable each draw

    // FP16 offscreen surface for HDR rendering (values > 1.0 preserved)
    auto info = SkImageInfo::Make(width_, height_,
                                  kRGBA_F16_SkColorType, kPremul_SkAlphaType,
                                  SkColorSpace::MakeSRGB());
    sceneSurface_ = SkSurfaces::RenderTarget(rec_.get(), info);
    if (!sceneSurface_) sceneSurface_ = skSurface_; // fallback to swapchain

    return sceneSurface_->getCanvas();
}

SkCanvas* SkiaRenderer::canvas() const {
    if (sceneSurface_ && sceneSurface_ != skSurface_)
        return sceneSurface_->getCanvas();
    return skSurface_ ? skSurface_->getCanvas() : nullptr;
}

void* SkiaRenderer::currentCanvas() const { return canvas(); }

void SkiaRenderer::present() {
    // Composite FP16 scene onto the swapchain, with optional bloom
    if (sceneSurface_ && sceneSurface_ != skSurface_) {
        auto sceneImg = sceneSurface_->makeImageSnapshot();
        if (sceneImg) {
            auto* dst = skSurface_->getCanvas();

            // 1. Tonemap + draw scene (> 1.0 values clamp to white on BGRA8 swapchain)
            dst->drawImage(sceneImg.get(), 0, 0, SkSamplingOptions());

            // 2. Multi-scale bloom: 3 passes at different radii, all composited additively.
            // Narrow pass = tight bright core, medium = glow body, wide = diffuse halo.
            // This matches the industry-standard approach for neon/laser effects.
            if (bloomStrength_ > 0.0f) {
                // Threshold: output=0 at input=1.0, only HDR (>1.0) survives
                const float k = 4.0f;
                float threshMat[20] = {
                    k, 0, 0, 0, -k,
                    0, k, 0, 0, -k,
                    0, 0, k, 0, -k,
                    0, 0, 0, 1,  0
                };
                auto threshFilter = SkImageFilters::ColorFilter(
                    SkColorFilters::Matrix(threshMat), nullptr);

                // Three passes: narrow (tight core), medium (glow body), wide (diffuse halo)
                // bloomStrength_ scales relative weights: narrow stays sharp, wide spreads more
                struct BloomPass { float sigma; float weight; };
                BloomPass passes[3] = {
                    { 4.0f,  bloomStrength_ * 2.0f },   // tight core
                    { 12.0f, bloomStrength_ * 3.0f },   // glow body
                    { 32.0f, bloomStrength_ * 2.0f },   // wide diffuse halo
                };

                for (auto& p : passes) {
                    float wMat[20] = {
                        p.weight, 0,        0,        0, 0,
                        0,        p.weight, 0,        0, 0,
                        0,        0,        p.weight, 0, 0,
                        0,        0,        0,        1, 0
                    };
                    auto weightFilter = SkImageFilters::ColorFilter(
                        SkColorFilters::Matrix(wMat), threshFilter);
                    auto blurFilter = SkImageFilters::Blur(
                        p.sigma, p.sigma, SkTileMode::kDecal, weightFilter);

                    SkPaint bloomPaint;
                    bloomPaint.setImageFilter(blurFilter);
                    bloomPaint.setBlendMode(SkBlendMode::kPlus);
                    dst->drawImage(sceneImg.get(), 0, 0, SkSamplingOptions(), &bloomPaint);
                }
            }
        }
        sceneSurface_.reset();
    }

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

void SkiaRenderer::setFillStyle(const std::string& c)   { state_.fillColor = parseColor4f(c); state_.fillGrad = -1; }
void SkiaRenderer::setStrokeStyle(const std::string& c) { state_.strokeColor = parseColor4f(c); }
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
void SkiaRenderer::setShadowColor(const std::string& c)  { state_.shadowColor = parseColor4f(c); }
void SkiaRenderer::setShadowBlur(float blur)             { state_.shadowBlur = blur; }
void SkiaRenderer::setShadowOffsetX(float x)             { state_.shadowOffsetX = x; }
void SkiaRenderer::setShadowOffsetY(float y)             { state_.shadowOffsetY = y; }
void SkiaRenderer::setBloom(float strength)              { bloomStrength_ = strength; }
double SkiaRenderer::getTime() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime_).count();
}

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
        grads_[id].stops.push_back({ offset, parseColor4f(color) });
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

// ── Clipping ──────────────────────────────────────────────────────────────────

void SkiaRenderer::clipRect(float x, float y, float w, float h) {
    if (auto* c = canvas()) c->clipRect(SkRect::MakeXYWH(x, y, w, h));
}
void SkiaRenderer::clipRoundRect(float x, float y, float w, float h, float r) {
    if (auto* c = canvas()) {
        SkRRect rr;
        rr.setRectXY(SkRect::MakeXYWH(x, y, w, h), r, r);
        c->clipRRect(rr, true); // true = anti-aliased
    }
}

// ── Images ────────────────────────────────────────────────────────────────────

void SkiaRenderer::registerImage(const std::string& name, const uint8_t* data, int size) {
    auto skData = SkData::MakeWithCopy(data, size);
    auto img = SkImages::DeferredFromEncodedData(skData);
    if (img) {
        images_[name] = img;
        fprintf(stderr, "[drawImage] registered: %s (%dx%d)\n", name.c_str(), img->width(), img->height());
    } else {
        fprintf(stderr, "[drawImage] failed to decode registered image: %s\n", name.c_str());
    }
}

void SkiaRenderer::drawImage(const std::string& name, float dx, float dy, float dw, float dh) {
    auto* c = canvas();
    if (!c) { fprintf(stderr, "[drawImage] no canvas\n"); return; }

    // Normalise path: strip leading "./" or directory components
    std::string key = std::filesystem::path(name).filename().string();

    auto it = images_.find(key);
    if (it == images_.end()) {
        // Try loading from the bundle's Resources directory (works in both Debug and Release)
        std::vector<std::string> candidates = {
            resourcePath_ + "/" + key,
            resourcePath_ + "/Images/" + key,
            resourcePath_ + "/Fonts/" + key,
        };
#ifndef NDEBUG
        // In Debug builds, also try the source directory
        candidates.push_back(std::string(UI_DIR) + "/" + key);
        candidates.push_back(name);
#endif
        for (auto& path : candidates) {
            auto skData = SkData::MakeFromFileName(path.c_str());
            if (skData) {
                fprintf(stderr, "[drawImage] loaded from: %s (%zu bytes)\n", path.c_str(), skData->size());
                auto img = SkImages::DeferredFromEncodedData(skData);
                if (img) {
                    auto gpuImg = SkImages::TextureFromImage(rec_.get(), img);
                    if (gpuImg)
                        images_[key] = gpuImg;
                    else
                        images_[key] = img;
                    it = images_.find(key);
                    break;
                }
            }
        }
        if (it == images_.end()) {
            fprintf(stderr, "[drawImage] failed to load: %s\n", name.c_str());
            return;
        }
    }

    SkRect dst = SkRect::MakeXYWH(dx, dy, dw, dh);

    // Ensure the image is GPU-backed for Skia Graphite.
    // If the cached image is still raster (e.g. from registerImage()), upload it
    // once and store the GPU texture back so subsequent frames are free.
    if (!it->second->isTextureBacked()) {
        auto gpuImage = SkImages::TextureFromImage(rec_.get(), it->second);
        if (gpuImage)
            it->second = gpuImage;
    }

    c->drawImageRect(it->second, dst, SkSamplingOptions(SkFilterMode::kLinear));
}
