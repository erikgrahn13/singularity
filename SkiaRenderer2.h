#pragma once

#include "IRenderer2.h"
#include <memory>
#include <string>
#include <functional>

#ifdef __linux__
class VulkanContext; // forward declaration — keeps X11/Vulkan headers out of here
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/core/SkColor.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include <vector>
#endif

class SkiaRenderer : public IRenderer {
public:
    explicit SkiaRenderer(std::string_view resourcePath);
    ~SkiaRenderer(); // defined in platform-specific .cpp where VulkanContext is complete

    void attachToWindow(IWindow& window) override;
    void* beginFrame() override;
    void* currentCanvas() const override;
    void present() override;

    void resize(int w, int h) override;

    void fillRect(float x, float y, float width, float height) override;
    void strokeRect(float x, float y, float width, float height) override;
    void clearRect(float x, float y, float width, float height) override;

    void beginPath(void* canvas) override;
    void moveTo(float x, float y) override;
    void lineTo(float x, float y) override;
    void closePath(void* canvas) override;
    void arc(float cx, float cy, float radius, float startAngle, float endAngle, bool anticlockwise = false) override;
    void arcTo(float x1, float y1, float x2, float y2, float radius) override;
    void quadraticCurveTo(float cpx, float cpy, float x, float y) override;
    void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) override;
    void ellipse(float cx, float cy, float rx, float ry, float rotation, float startAngle, float endAngle, bool anticlockwise = false) override;
    void rect(float x, float y, float w, float h) override;
    void roundRect(float x, float y, float w, float h, float r) override;

    void fill(void* canvas) override;
    void stroke(void* canvas) override;

    void fillText(const std::string& text, float x, float y) override;
    void strokeText(const std::string& text, float x, float y) override;
    float measureText(const std::string& text) override;

    void setFillStyle(const std::string& color) override;
    void setStrokeStyle(const std::string& color) override;
    void setLineWidth(float width) override;
    void setLineCap(const std::string& cap) override;
    void setFont(const std::string& font) override;
    void setGlobalAlpha(float alpha) override;
    void setTextAlign(const std::string& align) override;
    void setTextBaseline(const std::string& baseline) override;

    int createLinearGradient(float x0, float y0, float x1, float y1) override;
    int createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) override;
    void addColorStop(int id, float offset, const std::string& color, float hdr = 1.0f) override;
    void setFillStyleGradient(int id) override;
    void setStrokeStyleGradient(int id) override;

    void save(void* canvas) override;
    void restore(void* canvas) override;

    void translate(float x, float y) override;
    void rotate(float angle) override;
    void scale(float x, float y) override;
    void resetTransform(void* canvas) override;

private:
#ifdef __linux__
    // Draw state
    SkColor fillColor_   = SK_ColorWHITE;
    SkColor strokeColor_ = SK_ColorBLACK;
    float   lineWidth_   = 1.0f;
    float   globalAlpha_ = 1.0f;

    std::unique_ptr<VulkanContext>    vk_;
    sk_sp<GrDirectContext>            grContext_;
    std::vector<sk_sp<SkSurface>>    skiaSurfaces_;
    uint32_t                          currentImageIndex_ = 0;
    SkCanvas*                         currentCanvas_     = nullptr;
#endif
};