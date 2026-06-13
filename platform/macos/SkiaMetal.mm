// Metal Graphite backend stub for macOS — provides the factory + lifecycle methods.
// TODO: Implement full Metal Graphite rendering pipeline.
#include "../../SkiaRenderer2.h"
#include "../IWindow.h"
#include "include/ports/SkFontMgr_mac_ct.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkTypeface.h"

SkiaRenderer::SkiaRenderer(std::string_view) {
    fontMgr_  = SkFontMgr_New_CoreText(nullptr);
    typeface_ = fontMgr_->legacyMakeTypeface(nullptr, SkFontStyle());
    // TODO: Create Metal device, Graphite context + recorder
}

SkiaRenderer::~SkiaRenderer() {}

std::unique_ptr<IRenderer> IRenderer::createRenderer(std::string_view r) {
    return std::make_unique<SkiaRenderer>(r);
}

void SkiaRenderer::attachToWindow(IWindow& window) {
    // TODO: Create CAMetalLayer, attach to window.nativeHandle(), create swapchain surfaces
}

void* SkiaRenderer::beginFrame() {
    // TODO: Acquire next drawable, return canvas
    return nullptr;
}

void* SkiaRenderer::currentCanvas() const {
    return canvas();
}

void SkiaRenderer::present() {
    // TODO: snap recording, submit, present drawable
}

void SkiaRenderer::resize(int w, int h) {
    IRenderer::resize(w, h);
    // TODO: Recreate Metal surfaces
}
