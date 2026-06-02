#pragma once

#include "IRenderer2.h"
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "include/gpu/vk/VulkanExtensions.h"
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

    void  attachToWindow(IWindow& window) override;
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

private:
    // --- Vulkan ---
    VkInstance       instance_            = VK_NULL_HANDLE;
    VkPhysicalDevice physDev_             = VK_NULL_HANDLE;
    VkDevice         device_              = VK_NULL_HANDLE;
    VkQueue          queue_               = VK_NULL_HANDLE;
    uint32_t         graphicsQueueFamily_ = 0;
    VkSurfaceKHR     surface_             = VK_NULL_HANDLE;
    VkSwapchainKHR   swapchain_           = VK_NULL_HANDLE;
    VkFormat         swapFormat_          = VK_FORMAT_UNDEFINED;
    VkExtent2D       swapExtent_          = {};

    // --- Skia Graphite ---
    skgpu::VulkanExtensions                    extensions_;
    VkPhysicalDeviceFeatures2                  features2_{};
    std::unique_ptr<skgpu::graphite::Context>  ctx_;
    std::unique_ptr<skgpu::graphite::Recorder> rec_;

    struct Frame {
        VkImage          image  = VK_NULL_HANDLE;
        VkSemaphore      sem    = VK_NULL_HANDLE; // signalled when rendering is done
        sk_sp<SkSurface> surface;
    };
    std::vector<Frame> frames_;
    uint32_t           frameIdx_   = 0;
    VkSemaphore        acquireSem_ = VK_NULL_HANDLE;

    // --- Canvas 2D state ---
    struct State {
        SkColor       fillColor  = SK_ColorWHITE;
        SkColor       strokeColor= SK_ColorBLACK;
        float         lineWidth  = 1.0f;
        float         alpha      = 1.0f;
        SkPaint::Cap  lineCap    = SkPaint::kButt_Cap;
        SkPaint::Join lineJoin   = SkPaint::kMiter_Join;
        float         fontSize   = 16.0f;
        std::string   textAlign  = "left";
        std::string   textBase   = "alphabetic";
        int           fillGrad   = -1;
    };
    State              state_;
    std::vector<State> stack_;
    SkPathBuilder      path_;

    struct Gradient {
        enum class Type { Linear, Radial } type;
        float x0, y0, x1, y1, r0, r1;
        std::vector<std::pair<float, SkColor>> stops;
    };
    std::vector<Gradient> grads_;

    sk_sp<SkFontMgr>  fontMgr_;
    sk_sp<SkTypeface> typeface_;

    SkCanvas* canvas() const;
    void      recreateSwapchain(uint32_t w, uint32_t h);
    SkPaint   fillPaint() const;
    SkPaint   strokePaint() const;
};