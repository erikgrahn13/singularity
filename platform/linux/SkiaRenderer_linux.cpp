// Linux-specific implementation of SkiaRenderer.
// This is the only file that needs to know about Vulkan + Skia-Vulkan bridge.
// Future: platform/macos/SkiaRenderer_mac.cpp (Metal), platform/windows/SkiaRenderer_win.cpp (D3D12)

#include "../../SkiaRenderer2.h"
#include "VulkanContext.h"
#include "include/gpu/ganesh/vk/GrVkDirectContext.h"
#include "include/gpu/ganesh/vk/GrVkBackendSurface.h"
#include "include/gpu/ganesh/vk/GrVkTypes.h"
#include "include/gpu/ganesh/vk/GrVkBackendSemaphore.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrBackendSemaphore.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/GrTypes.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/gpu/vk/VulkanMutableTextureState.h"
#include "include/gpu/MutableTextureState.h"
#include "include/core/SkColorSpace.h"
#include <stdexcept>

SkiaRenderer::SkiaRenderer(std::string_view resourcePath) {}

SkiaRenderer::~SkiaRenderer() = default;

std::unique_ptr<IRenderer> IRenderer::createRenderer(std::string_view resourcePath) {
    return std::make_unique<SkiaRenderer>(resourcePath);
}

void SkiaRenderer::attachToWindow(IWindow& window) {
    vk_ = std::make_unique<VulkanContext>();
    vk_->init(window);

    // --- Step 5: Create Skia's GrDirectContext backed by our Vulkan device ---
    // Skia resolves every Vulkan function it needs at runtime through fGetProc,
    // rather than linking against libvulkan itself.
    skgpu::VulkanBackendContext vkCtx{};
    vkCtx.fInstance           = vk_->instance();
    vkCtx.fPhysicalDevice     = vk_->physicalDevice();
    vkCtx.fDevice             = vk_->device();
    vkCtx.fQueue              = vk_->graphicsQueue();
    vkCtx.fGraphicsQueueIndex = vk_->graphicsQueueFamily();
    vkCtx.fGetProc = [](const char* name, VkInstance inst, VkDevice dev) -> PFN_vkVoidFunction {
        if (dev != VK_NULL_HANDLE) return vkGetDeviceProcAddr(dev, name);
        return vkGetInstanceProcAddr(inst, name); // works even when inst is VK_NULL_HANDLE (global functions)
    };

    grContext_ = GrDirectContexts::MakeVulkan(vkCtx);
    if (!grContext_)
        throw std::runtime_error("GrDirectContexts::MakeVulkan failed");

    // --- Step 7: Wrap each swapchain VkImage as an SkSurface ---
    // Skia needs a GrVkImageInfo describing the image's format and state,
    // then a GrBackendRenderTarget, then the SkSurface wrapper.
    const auto& images = vk_->swapchainImages();
    const VkExtent2D ext = vk_->swapchainExtent();
    skiaSurfaces_.reserve(images.size());

    for (VkImage image : images) {
        GrVkImageInfo imageInfo{};
        imageInfo.fImage              = image;
        imageInfo.fImageLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.fFormat             = vk_->swapchainFormat(); // VK_FORMAT_B8G8R8A8_UNORM
        imageInfo.fSampleCount        = 1;
        imageInfo.fLevelCount         = 1;
        imageInfo.fCurrentQueueFamily = vk_->graphicsQueueFamily();

        GrBackendRenderTarget backendRT =
            GrBackendRenderTargets::MakeVk(
                static_cast<int>(ext.width),
                static_cast<int>(ext.height),
                imageInfo);

        // kTopLeft matches Vulkan's top-left origin.
        // kBGRA_8888 matches VK_FORMAT_B8G8R8A8_UNORM.
        // nullptr colorSpace = no colour management (raw sRGB).
        sk_sp<SkSurface> surface =
            SkSurfaces::WrapBackendRenderTarget(
                grContext_.get(),
                backendRT,
                kTopLeft_GrSurfaceOrigin,
                kBGRA_8888_SkColorType,
                nullptr,
                nullptr);

        if (!surface)
            throw std::runtime_error("SkSurfaces::WrapBackendRenderTarget failed");

        skiaSurfaces_.push_back(std::move(surface));
    }
}

void* SkiaRenderer::beginFrame() {
    VkResult result = vkAcquireNextImageKHR(
        vk_->device(),
        vk_->swapchain(),
        UINT64_MAX,
        vk_->acquireSemaphore(),
        VK_NULL_HANDLE,
        &currentImageIndex_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain is stale — recreate it and all Skia surfaces.
        vkDeviceWaitIdle(vk_->device());
        vk_->createSwapchain(static_cast<uint32_t>(width_), static_cast<uint32_t>(height_));

        const auto& images = vk_->swapchainImages();
        const VkExtent2D ext = vk_->swapchainExtent();
        skiaSurfaces_.clear();
        for (VkImage image : images) {
            GrVkImageInfo imageInfo{};
            imageInfo.fImage              = image;
            imageInfo.fImageLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.fFormat             = vk_->swapchainFormat();
            imageInfo.fSampleCount        = 1;
            imageInfo.fLevelCount         = 1;
            imageInfo.fCurrentQueueFamily = vk_->graphicsQueueFamily();

            GrBackendRenderTarget backendRT =
                GrBackendRenderTargets::MakeVk(
                    static_cast<int>(ext.width),
                    static_cast<int>(ext.height),
                    imageInfo);

            sk_sp<SkSurface> surface =
                SkSurfaces::WrapBackendRenderTarget(
                    grContext_.get(), backendRT,
                    kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType,
                    nullptr, nullptr);
            if (!surface) return nullptr;
            skiaSurfaces_.push_back(std::move(surface));
        }

        // Retry acquire with the fresh swapchain.
        result = vkAcquireNextImageKHR(
            vk_->device(), vk_->swapchain(), UINT64_MAX,
            vk_->acquireSemaphore(), VK_NULL_HANDLE, &currentImageIndex_);
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        currentCanvas_ = nullptr;
        return nullptr;
    }

    currentCanvas_ = skiaSurfaces_[currentImageIndex_]->getCanvas();
    return currentCanvas_;
}

void SkiaRenderer::present() {
    if (!currentCanvas_) return;
    // Tell Skia to flush all pending draw commands and transition the image
    // layout to PRESENT_SRC so Vulkan can hand it to the display.
    // We ask Skia to signal renderSemaphore_ when the GPU work is done.
    GrBackendSemaphore signalSem = GrBackendSemaphores::MakeVk(vk_->renderSemaphore());

    GrFlushInfo flushInfo{};
    flushInfo.fNumSemaphores    = 1;
    flushInfo.fSignalSemaphores = &signalSem;

    skgpu::MutableTextureState presentState =
        skgpu::MutableTextureStates::MakeVulkan(
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            vk_->graphicsQueueFamily());

    grContext_->flush(skiaSurfaces_[currentImageIndex_].get(), flushInfo, &presentState);
    grContext_->submit();

    // Hand the image back to the display, waiting until rendering is done.
    VkSemaphore waitSem = vk_->renderSemaphore();
    VkSwapchainKHR sc   = vk_->swapchain();

    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &waitSem;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &sc;
    presentInfo.pImageIndices      = &currentImageIndex_;

    vkQueuePresentKHR(vk_->graphicsQueue(), &presentInfo);
}
