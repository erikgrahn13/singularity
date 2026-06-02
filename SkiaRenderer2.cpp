#include "SkiaRenderer2.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/gpu/vk/VulkanPreferredFeatures.h"
#include "include/gpu/graphite/vk/VulkanGraphiteContext.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/Recording.h"
#include "include/gpu/graphite/GraphiteTypes.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPath.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/utils/SkTextUtils.h"
#include "include/effects/SkGradientShader.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/graphite/vk/VulkanGraphiteTypes.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/gpu/graphite/BackendSemaphore.h"
#include "include/gpu/MutableTextureState.h"
#include "include/gpu/vk/VulkanMutableTextureState.h"
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#ifdef __linux__
#include "platform/linux/X11Window.h"
#include <vulkan/vulkan_xlib.h>
#endif
#include <algorithm>
#include <stdexcept>

// Graphite requires VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT alongside COLOR_ATTACHMENT
// for a texture to be considered renderable. See VulkanCaps::getTextureUsage.
static constexpr VkImageUsageFlags kSwapUsage =
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT          |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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

SkCanvas* SkiaRenderer::canvas() const {
    return frames_.empty() ? nullptr : frames_[frameIdx_].surface->getCanvas();
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

SkiaRenderer::SkiaRenderer(std::string_view) {
    // Use whatever version the driver actually supports, capped at 1.4.
    // vkEnumerateInstanceVersion is core since Vulkan 1.1.
    uint32_t kApiVer = VK_API_VERSION_1_1;
    if (vkEnumerateInstanceVersion(&kApiVer) != VK_SUCCESS)
        kApiVer = VK_API_VERSION_1_1;
    kApiVer = std::min(kApiVer, (uint32_t)VK_API_VERSION_1_4);

    skgpu::VulkanPreferredFeatures skiaFeatures;
    skiaFeatures.init(kApiVer);

    // Instance extensions
    uint32_t n = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr);
    std::vector<VkExtensionProperties> instProps(n);
    vkEnumerateInstanceExtensionProperties(nullptr, &n, instProps.data());

    std::vector<const char*> instExts = { VK_KHR_SURFACE_EXTENSION_NAME };
#ifdef __linux__
    instExts.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(_WIN32)
    instExts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__APPLE__)
    instExts.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
    skiaFeatures.addToInstanceExtensions(instProps.data(), n, instExts);

    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = kApiVer;
    VkInstanceCreateInfo instCI{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instCI.pApplicationInfo        = &appInfo;
    instCI.enabledExtensionCount   = (uint32_t)instExts.size();
    instCI.ppEnabledExtensionNames = instExts.data();
    if (vkCreateInstance(&instCI, nullptr, &instance_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");

    // Physical device — prefer discrete GPU
    uint32_t gpuN = 0;
    vkEnumeratePhysicalDevices(instance_, &gpuN, nullptr);
    std::vector<VkPhysicalDevice> gpus(gpuN);
    vkEnumeratePhysicalDevices(instance_, &gpuN, gpus.data());
    physDev_ = gpus[0];
    for (auto g : gpus) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(g, &p);
        printf("GPU: %s\n", p.deviceName);
        if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) physDev_ = g;
    }

    // Graphics queue family
    uint32_t qfN = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDev_, &qfN, nullptr);
    std::vector<VkQueueFamilyProperties> qfs(qfN);
    vkGetPhysicalDeviceQueueFamilyProperties(physDev_, &qfN, qfs.data());
    for (uint32_t i = 0; i < qfN; i++)
        if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { graphicsQueueFamily_ = i; break; }

    // Device extensions + Skia feature chain
    uint32_t devN = 0;
    vkEnumerateDeviceExtensionProperties(physDev_, nullptr, &devN, nullptr);
    std::vector<VkExtensionProperties> devProps(devN);
    vkEnumerateDeviceExtensionProperties(physDev_, nullptr, &devN, devProps.data());
    std::vector<const char*> devExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    features2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    skiaFeatures.addFeaturesToQuery(devProps.data(), devN, features2_);
    vkGetPhysicalDeviceFeatures2(physDev_, &features2_);
    skiaFeatures.addFeaturesToEnable(devExts, features2_);

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qCI{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qCI.queueFamilyIndex = graphicsQueueFamily_;
    qCI.queueCount       = 1;
    qCI.pQueuePriorities = &prio;
    VkDeviceCreateInfo devCI{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    devCI.pNext                   = &features2_;
    devCI.queueCreateInfoCount    = 1;
    devCI.pQueueCreateInfos       = &qCI;
    devCI.enabledExtensionCount   = (uint32_t)devExts.size();
    devCI.ppEnabledExtensionNames = devExts.data();
    if (vkCreateDevice(physDev_, &devCI, nullptr, &device_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed");
    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &queue_);

    // Skia Graphite context
    auto getProc = [](const char* name, VkInstance inst, VkDevice dev) -> PFN_vkVoidFunction {
        return dev ? vkGetDeviceProcAddr(dev, name) : vkGetInstanceProcAddr(inst, name);
    };
    extensions_.init(getProc, instance_, physDev_,
                     (uint32_t)instExts.size(), instExts.data(),
                     (uint32_t)devExts.size(),  devExts.data());

    skgpu::VulkanBackendContext bc{};
    bc.fInstance           = instance_;
    bc.fPhysicalDevice     = physDev_;
    bc.fDevice             = device_;
    bc.fQueue              = queue_;
    bc.fGraphicsQueueIndex = graphicsQueueFamily_;
    bc.fMaxAPIVersion      = kApiVer;
    bc.fVkExtensions       = &extensions_;
    bc.fDeviceFeatures2    = &features2_;
    bc.fGetProc            = getProc;

    skgpu::graphite::ContextOptions ctxOpts{};
    ctx_ = skgpu::graphite::ContextFactory::MakeVulkan(bc, ctxOpts);
    if (!ctx_) throw std::runtime_error("ContextFactory::MakeVulkan failed");
    rec_ = ctx_->makeRecorder();

    fontMgr_  = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
    typeface_ = fontMgr_->legacyMakeTypeface(nullptr, SkFontStyle());
}

std::unique_ptr<IRenderer> IRenderer::createRenderer(std::string_view r) {
    return std::make_unique<SkiaRenderer>(r);
}

// ── Swapchain ─────────────────────────────────────────────────────────────────

void SkiaRenderer::recreateSwapchain(uint32_t w, uint32_t h) {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev_, surface_, &caps);

    swapExtent_ = (caps.currentExtent.width != UINT32_MAX)
                ? caps.currentExtent
                : VkExtent2D{
                    std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width),
                    std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height) };

    uint32_t imgCount = std::min(caps.minImageCount + 1,
                                 caps.maxImageCount > 0 ? caps.maxImageCount : UINT32_MAX);

    // Pick format — prefer BGRA8_UNORM
    uint32_t fmtN = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev_, surface_, &fmtN, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmtN);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev_, surface_, &fmtN, fmts.data());
    VkSurfaceFormatKHR fmt = fmts[0];
    for (auto& f : fmts)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { fmt = f; break; }
    swapFormat_ = fmt.format;

    // Pick present mode — prefer MAILBOX (low-latency), fallback FIFO (always available)
    uint32_t modeN = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDev_, surface_, &modeN, nullptr);
    std::vector<VkPresentModeKHR> modes(modeN);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDev_, surface_, &modeN, modes.data());
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto& m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) { mode = m; break; }

    VkSwapchainKHR old = swapchain_;
    VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    sci.surface          = surface_;
    sci.minImageCount    = imgCount;
    sci.imageFormat      = fmt.format;
    sci.imageColorSpace  = fmt.colorSpace;
    sci.imageExtent      = swapExtent_;
    sci.imageArrayLayers = 1;
    sci.imageUsage       = kSwapUsage;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform     = caps.currentTransform;
    sci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode      = mode;
    sci.clipped          = VK_TRUE;
    sci.oldSwapchain     = old;
    if (vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateSwapchainKHR failed");
    if (old) vkDestroySwapchainKHR(device_, old, nullptr);

    // Get VkImages and wrap each one as a Skia surface
    uint32_t imgN = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &imgN, nullptr);
    std::vector<VkImage> vkImgs(imgN);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imgN, vkImgs.data());

    SkColorType ct = (swapFormat_ == VK_FORMAT_R8G8B8A8_UNORM)
                   ? kRGBA_8888_SkColorType : kBGRA_8888_SkColorType;

    frames_.resize(imgN);
    for (uint32_t i = 0; i < imgN; ++i) {
        frames_[i].image = vkImgs[i];

        VkSemaphoreCreateInfo sCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(device_, &sCI, nullptr, &frames_[i].sem);

        skgpu::graphite::VulkanTextureInfo ti{};
        ti.fFormat          = swapFormat_;
        ti.fImageTiling     = VK_IMAGE_TILING_OPTIMAL;
        ti.fImageUsageFlags = kSwapUsage;
        ti.fSharingMode     = VK_SHARING_MODE_EXCLUSIVE;

        skgpu::graphite::BackendTexture bt =
            skgpu::graphite::BackendTextures::MakeVulkan(
                { (int32_t)swapExtent_.width, (int32_t)swapExtent_.height },
                ti, VK_IMAGE_LAYOUT_UNDEFINED, graphicsQueueFamily_,
                frames_[i].image, skgpu::VulkanAlloc{});

        frames_[i].surface = SkSurfaces::WrapBackendTexture(rec_.get(), bt, ct, nullptr, nullptr);
        if (!frames_[i].surface)
            throw std::runtime_error("WrapBackendTexture failed");
    }
}

// ── Window attachment ─────────────────────────────────────────────────────────

void SkiaRenderer::attachToWindow(IWindow& window) {
#ifdef __linux__
    auto& x = static_cast<X11Window&>(window);
    VkXlibSurfaceCreateInfoKHR sCI{ VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
    sCI.dpy    = x.display();
    sCI.window = x.xwindow();
    if (vkCreateXlibSurfaceKHR(instance_, &sCI, nullptr, &surface_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateXlibSurfaceKHR failed");
#endif
    recreateSwapchain(window.width(), window.height());
}

// ── Destructor ────────────────────────────────────────────────────────────────

SkiaRenderer::~SkiaRenderer() {
    if (!device_) return;
    vkDeviceWaitIdle(device_);
    for (auto& f : frames_) {
        f.surface.reset();
        if (f.sem) vkDestroySemaphore(device_, f.sem, nullptr);
    }
    if (acquireSem_) vkDestroySemaphore(device_, acquireSem_, nullptr);
    rec_.reset();
    ctx_.reset();
    if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    if (surface_)   vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

// ── Frame lifecycle ───────────────────────────────────────────────────────────

void SkiaRenderer::resize(int w, int h) {
    IRenderer::resize(w, h);
    if (!swapchain_) return;
    vkDeviceWaitIdle(device_);
    for (auto& f : frames_) {
        f.surface.reset();
        if (f.sem) vkDestroySemaphore(device_, f.sem, nullptr);
    }
    frames_.clear();
    recreateSwapchain(w, h);
}

void* SkiaRenderer::beginFrame() {
    VkSemaphoreCreateInfo sCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(device_, &sCI, nullptr, &acquireSem_);

    VkResult r = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                       acquireSem_, VK_NULL_HANDLE, &frameIdx_);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        vkDestroySemaphore(device_, acquireSem_, nullptr);
        acquireSem_ = VK_NULL_HANDLE;
        resize(width_, height_);
        return beginFrame();
    }
    return frames_[frameIdx_].surface->getCanvas();
}

void* SkiaRenderer::currentCanvas() const { return canvas(); }

void SkiaRenderer::present() {
    auto recording = rec_->snap();
    if (!recording) return;

    auto acqSem = skgpu::graphite::BackendSemaphores::MakeVulkan(acquireSem_);
    auto renSem = skgpu::graphite::BackendSemaphores::MakeVulkan(frames_[frameIdx_].sem);
    auto layout = skgpu::MutableTextureStates::MakeVulkan(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                          graphicsQueueFamily_);
    skgpu::graphite::InsertRecordingInfo info{};
    info.fRecording          = recording.get();
    info.fTargetSurface      = frames_[frameIdx_].surface.get();
    info.fTargetTextureState = &layout;
    info.fNumWaitSemaphores   = 1;  info.fWaitSemaphores   = &acqSem;
    info.fNumSignalSemaphores = 1;  info.fSignalSemaphores = &renSem;

    // Destroy the acquire semaphore once the GPU is done with it
    struct Done { VkDevice dev; VkSemaphore sem; };
    auto* done = new Done{ device_, acquireSem_ };
    info.fFinishedContext = done;
    info.fFinishedProc = [](skgpu::graphite::GpuFinishedContext c, skgpu::CallbackResult) {
        auto* d = static_cast<Done*>(c);
        vkDestroySemaphore(d->dev, d->sem, nullptr);
        delete d;
    };
    acquireSem_ = VK_NULL_HANDLE;

    ctx_->insertRecording(info);
    ctx_->submit(skgpu::graphite::SyncToCpu::kNo);

    VkSemaphore waitSem = frames_[frameIdx_].sem;
    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;  pi.pWaitSemaphores = &waitSem;
    pi.swapchainCount     = 1;  pi.pSwapchains     = &swapchain_;
    pi.pImageIndices      = &frameIdx_;
    vkQueuePresentKHR(queue_, &pi);
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
