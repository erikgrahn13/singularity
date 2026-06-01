#include "VulkanContext.h"
#include "X11Window.h"
#include <stdexcept>
#include <algorithm>

VulkanContext::~VulkanContext() {
    // Destroy in reverse creation order
    if (acquireSemaphore_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, acquireSemaphore_, nullptr);
    if (renderSemaphore_  != VK_NULL_HANDLE) vkDestroySemaphore(device_, renderSemaphore_,  nullptr);
    if (swapchain_ != VK_NULL_HANDLE) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    if (surface_   != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (device_    != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);
    if (instance_  != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
}

void VulkanContext::init(IWindow& iwindow) {
    auto& x11 = static_cast<X11Window&>(iwindow);
    Display*  display = x11.display();
    ::Window  window  = x11.xwindow();

    // --- Step 1: Create VkInstance with surface extensions ---
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    };

    VkApplicationInfo appInfo{};
    appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo        = &appInfo;
    instanceInfo.enabledExtensionCount   = 2;
    instanceInfo.ppEnabledExtensionNames = instanceExtensions;

    if (vkCreateInstance(&instanceInfo, nullptr, &instance_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");

    // --- Step 2: Pick a physical device (GPU) ---
    // Prefer a discrete GPU, fall back to the first device found.
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    physicalDevice_ = devices[0];
    for (auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physicalDevice_ = dev;
            break;
        }
    }

    // --- Step 3: Find graphics queue family and create logical device ---
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());

    graphicsQueueFamily_ = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily_ = i;
            break;
        }
    }
    if (graphicsQueueFamily_ == UINT32_MAX)
        throw std::runtime_error("No graphics queue family found");

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = graphicsQueueFamily_;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount    = 1;
    deviceInfo.pQueueCreateInfos       = &queueInfo;
    deviceInfo.enabledExtensionCount   = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed");

    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);

    // --- Step 4: Create Vulkan surface from the X11 window ---
    // This connects Vulkan to the OS window so we can present rendered frames.
    VkXlibSurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.dpy    = display;
    surfaceInfo.window = window;

    if (vkCreateXlibSurfaceKHR(instance_, &surfaceInfo, nullptr, &surface_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateXlibSurfaceKHR failed");

    createSwapchain(static_cast<uint32_t>(iwindow.width()),
                    static_cast<uint32_t>(iwindow.height()));

    // Create the two synchronisation semaphores.
    // acquireSemaphore_: GPU waits on this before rendering — signals when the
    //                    swapchain image is no longer being displayed.
    // renderSemaphore_:  GPU signals this when rendering is done — vkQueuePresentKHR
    //                    waits on it before showing the image.
    VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    if (vkCreateSemaphore(device_, &semInfo, nullptr, &acquireSemaphore_) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &semInfo, nullptr, &renderSemaphore_)  != VK_SUCCESS)
        throw std::runtime_error("vkCreateSemaphore failed");
}

void VulkanContext::createSwapchain(uint32_t width, uint32_t height) {
    // --- Step 6a: Query what the surface supports ---
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &caps);

    // --- Step 6b: Choose a surface format ---
    // We want BGRA8 UNORM — the most universally supported format on Linux/X11.
    // Skia manages its own colour management so we use UNORM, not SRGB.
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

    swapchainFormat_ = formats[0].format; // fallback
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchainFormat_ = f.format;
            break;
        }
    }

    // --- Step 6c: Choose a present mode ---
    // MAILBOX = triple-buffered, lowest latency. FIFO = vsync, always available.
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &modeCount, modes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // always supported
    for (const auto& m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) { presentMode = m; break; }
    }

    // --- Step 6d: Determine swap extent ---
    // If currentExtent is UINT32_MAX the surface lets us choose; otherwise we
    // must match the reported size exactly.
    if (caps.currentExtent.width != UINT32_MAX) {
        swapchainExtent_ = caps.currentExtent;
    } else {
        swapchainExtent_.width  = std::clamp(width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
        swapchainExtent_.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // --- Step 6e: Create the swapchain ---
    // Request one extra image beyond the minimum so the GPU can start the next
    // frame while the previous one is still being shown.
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0)
        imageCount = std::min(imageCount, caps.maxImageCount);

    VkSwapchainKHR oldSwapchain = swapchain_; // VK_NULL_HANDLE on first call

    VkSwapchainCreateInfoKHR sci{};
    sci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface          = surface_;
    sci.minImageCount    = imageCount;
    sci.imageFormat      = swapchainFormat_;
    sci.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent      = swapchainExtent_;
    sci.imageArrayLayers = 1;
    sci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // one queue family — no sharing needed
    sci.preTransform     = caps.currentTransform;
    sci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode      = presentMode;
    sci.clipped          = VK_TRUE;
    sci.oldSwapchain     = oldSwapchain;

    if (vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_) != VK_SUCCESS)
        throw std::runtime_error("vkCreateSwapchainKHR failed");

    if (oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device_, oldSwapchain, nullptr);

    // --- Step 6f: Retrieve the VkImage handles ---
    // These are the actual GPU images we'll render into.
    uint32_t actualCount = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &actualCount, nullptr);
    swapchainImages_.resize(actualCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &actualCount, swapchainImages_.data());
}
