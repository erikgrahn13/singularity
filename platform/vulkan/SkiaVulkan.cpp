// Vulkan Graphite backend for SkiaRenderer (Linux + Windows)
#include "../../SkiaRenderer2.h"
#include "../IWindow.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/gpu/vk/VulkanPreferredFeatures.h"
#include "include/gpu/graphite/vk/VulkanGraphiteContext.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/Recording.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/graphite/vk/VulkanGraphiteTypes.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/gpu/graphite/BackendSemaphore.h"
#include "include/gpu/MutableTextureState.h"
#include "include/gpu/vk/VulkanMutableTextureState.h"

#ifdef __linux__
#include "../linux/X11Window.h"
#include <vulkan/vulkan_xlib.h>
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#elif defined(_WIN32)
#include <vulkan/vulkan_win32.h>
#endif

#include <algorithm>
#include <stdexcept>

// Graphite requires VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT alongside COLOR_ATTACHMENT
static constexpr VkImageUsageFlags kSwapUsage =
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT          |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT;

// ── Construction ─────────────────────────────────────────────────────────────

SkiaRenderer::SkiaRenderer(std::string_view) {
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

#ifdef __linux__
    fontMgr_  = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#elif defined(_WIN32)
    // TODO: Windows font manager
#endif
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

    uint32_t fmtN = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev_, surface_, &fmtN, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmtN);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev_, surface_, &fmtN, fmts.data());
    VkSurfaceFormatKHR fmt = fmts[0];
    for (auto& f : fmts)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { fmt = f; break; }
    swapFormat_ = fmt.format;

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
#elif defined(_WIN32)
    // TODO: Win32 surface creation
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
