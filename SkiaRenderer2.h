#pragma once

#include "IRenderer2.h"
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dawn/webgpu_cpp.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/Recorder.h"
#include "include/core/SkColor.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPaint.h"
#include "include/core/SkFontMgr.h"

class SkiaRenderer : public IRenderer {
public:
    explicit SkiaRenderer(std::string_view resourcePath);
    ~SkiaRenderer();

    void attachToWindow(IWindow& window) override;
    void* beginFrame() override;
    void* currentCanvas() const override;
    void  present() override;
    void  resize(int w, int h) override;

    void  fillRect(float x, float y, float w, float h) override;
    void  strokeRect(float x, float y, float w, float h) override;
    void  clearRect(float x, float y, float w, float h) override;
    void  beginPath(void*) override;
    void  moveTo(float x, float y) override;
    void  lineTo(float x, float y) override;
    void  closePath(void*) override;
    void  arc(float cx, float cy, float r, float start, float end, bool ccw = false) override;
    void  arcTo(float x1, float y1, float x2, float y2, float r) override;
    void  quadraticCurveTo(float cpx, float cpy, float x, float y) override;
    void  bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) override;
    void  ellipse(float cx, float cy, float rx, float ry, float rot, float start, float end, bool ccw = false) override;
    void  rect(float x, float y, float w, float h) override;
    void  roundRect(float x, float y, float w, float h, float r) override;
    void  fill(void*) override;
    void  stroke(void*) override;
    void  fillText(const std::string& text, float x, float y) override;
    void  strokeText(const std::string& text, float x, float y) override;
    float measureText(const std::string& text) override;
    void  setFillStyle(const std::string& color) override;
    void  setStrokeStyle(const std::string& color) override;
    void  setLineWidth(float w) override;
    void  setLineCap(const std::string& cap) override;
    void  setFont(const std::string& font) override;
    void  setGlobalAlpha(float a) override;
    void  setTextAlign(const std::string& align) override;
    void  setTextBaseline(const std::string& b) override;
    void  setShadowColor(const std::string& color) override;
    void  setShadowBlur(float blur) override;
    void  setShadowOffsetX(float x) override;
    void  setShadowOffsetY(float y) override;
    void  setBloom(float strength) override;
    double getTime() const override;
    int   createLinearGradient(float x0, float y0, float x1, float y1) override;
    int   createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) override;
    void  addColorStop(int id, float offset, const std::string& color, float hdr = 1.0f) override;
    void  setFillStyleGradient(int id) override;
    void  setStrokeStyleGradient(int id) override;
    void  save(void*) override;
    void  restore(void*) override;
    void  translate(float x, float y) override;
    void  rotate(float angle) override;
    void  scale(float x, float y) override;
    void  resetTransform(void*) override;
    void  clipRect(float x, float y, float w, float h) override;
    void  clipRoundRect(float x, float y, float w, float h, float r) override;

    void  registerImage(const std::string& name, const uint8_t* data, int size) override;
    void  drawImage(const std::string& name, float dx, float dy, float dw, float dh) override;

private:
    wgpu::Instance   instance_;
    wgpu::Adapter    adapter_;
    wgpu::Device     device_;
    wgpu::Surface    surface_;
    wgpu::TextureFormat swapFormat_ = wgpu::TextureFormat::Undefined;
    std::unique_ptr<skgpu::graphite::Context>  ctx_;
    std::unique_ptr<skgpu::graphite::Recorder> rec_;
    sk_sp<SkSurface> skSurface_;
    // --- Canvas 2D state ---
    struct State {
        SkColor4f     fillColor    = {1,1,1,1};
        SkColor4f     strokeColor  = {0,0,0,1};
        float         lineWidth    = 1.0f;
        float         alpha        = 1.0f;
        SkPaint::Cap  lineCap      = SkPaint::kButt_Cap;
        SkPaint::Join lineJoin     = SkPaint::kMiter_Join;
        float         fontSize     = 16.0f;
        std::string   fontFamily;
        std::string   textAlign    = "left";
        std::string   textBase     = "alphabetic";
        int           fillGrad     = -1;
        // Shadow / glow
        SkColor4f     shadowColor   = {0,0,0,0};
        float         shadowBlur    = 0.0f;
        float         shadowOffsetX = 0.0f;
        float         shadowOffsetY = 0.0f;
    };
    State              state_;
    std::vector<State> stack_;
    SkPathBuilder      path_;

    struct Gradient {
        enum class Type { Linear, Radial } type;
        float x0, y0, x1, y1, r0, r1;
        std::vector<std::pair<float, SkColor4f>> stops;
    };
    std::vector<Gradient> grads_;

    sk_sp<SkFontMgr>  fontMgr_;
    sk_sp<SkTypeface> typeface_;
    std::map<std::string, sk_sp<SkTypeface>> loadedTypefaces_;
    sk_sp<SkSurface>  sceneSurface_; // FP16 offscreen for HDR rendering
    float             bloomStrength_ = 0.0f; // off by default, set via ctx.bloom
    std::chrono::steady_clock::time_point startTime_ = std::chrono::steady_clock::now();

    std::map<std::string, sk_sp<SkImage>> images_;
    std::string resourcePath_;

    SkCanvas* canvas() const;
    SkPaint   fillPaint() const;
    SkPaint   strokePaint() const;
    void      applyShadow(SkPaint& p) const;
    sk_sp<SkTypeface> resolveTypeface();
};