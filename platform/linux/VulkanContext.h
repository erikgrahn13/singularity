#pragma once
#include <X11/Xlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vector>
#include "../IWindow.h"

// Owns raw Vulkan objects: instance, physical device, logical device,
// graphics queue, and window surface. Completely Skia-agnostic.
class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    // Steps 1-4: create instance, pick GPU, create device/queue, create surface.
    void init(IWindow& window);

    // Step 6: create (or recreate) the swapchain. Call on init and on resize.
    void createSwapchain(uint32_t width, uint32_t height);

    VkInstance       instance()            const { return instance_; }
    VkPhysicalDevice physicalDevice()      const { return physicalDevice_; }
    VkDevice         device()              const { return device_; }
    VkQueue          graphicsQueue()       const { return graphicsQueue_; }
    uint32_t         graphicsQueueFamily() const { return graphicsQueueFamily_; }
    VkSurfaceKHR     surface()             const { return surface_; }

    VkSwapchainKHR              swapchain()       const { return swapchain_; }
    VkFormat                    swapchainFormat() const { return swapchainFormat_; }
    VkExtent2D                  swapchainExtent() const { return swapchainExtent_; }
    const std::vector<VkImage>& swapchainImages() const { return swapchainImages_; }

    // One semaphore signals when the swapchain image is ready to render into;
    // the other signals when rendering is done and the image can be presented.
    VkSemaphore acquireSemaphore() const { return acquireSemaphore_; }
    VkSemaphore renderSemaphore()  const { return renderSemaphore_; }

private:
    VkInstance       instance_            = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_      = VK_NULL_HANDLE;
    VkDevice         device_              = VK_NULL_HANDLE;
    VkQueue          graphicsQueue_       = VK_NULL_HANDLE;
    uint32_t         graphicsQueueFamily_ = 0;
    VkSurfaceKHR     surface_             = VK_NULL_HANDLE;

    VkSwapchainKHR       swapchain_       = VK_NULL_HANDLE;
    VkFormat             swapchainFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D           swapchainExtent_ = {};
    std::vector<VkImage> swapchainImages_;

    VkSemaphore          acquireSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore          renderSemaphore_  = VK_NULL_HANDLE;
};
