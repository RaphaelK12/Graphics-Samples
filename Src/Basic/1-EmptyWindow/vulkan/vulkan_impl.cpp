//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <vector>
#include <windows.h>
#include <memory>
#include "vulkan/vulkan.h"
#include "vulkan_impl.h"

// Vulkan properties
std::vector<VkLayerProperties>  g_vk_properties;
// Vulkan extensions
std::vector<const char*>        g_vk_device_exts;
// Vulkan instance extentions
std::vector<const char*>        g_vk_instance_exts;
// This is the vulkan instance
VkInstance              g_instance;
// vulkan device
VkDevice                g_device;
// vulkan command pool
VkCommandPool           g_commnad_pool;
// vulkan command
VkCommandBuffer         g_command;
// swap chain surface
VkSurfaceKHR            g_surface;
// back buffer format
VkFormat                g_back_buffer_format;
// the swap chain
VkSwapchainKHR          g_swapchain;
// swap chain image count
uint32_t                g_swapchain_image_count;

typedef struct _swap_chain_buffers {
    VkImage image;
    VkImageView view;
} swap_chain_buffer;

std::vector<swap_chain_buffer>       g_swapchain_buffers;

typedef struct {
    VkLayerProperties properties;
    std::vector<VkExtensionProperties> instance_extensions;
    std::vector<VkExtensionProperties> device_extensions;
} layer_properties;

static VkResult init_global_extension_properties(layer_properties& layer_props) {
    VkExtensionProperties* instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char* layer_name = NULL;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res) return res;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.instance_extensions.resize(instance_extension_count);
        instance_extensions = layer_props.instance_extensions.data();
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

bool VulkanGraphicsSample::initialize(const HINSTANCE hInstnace, const HWND hwnd) {
    // initialize vulkan properties
    VkResult res;
    uint32_t instance_layer_count;
    do {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (res) return res;

        if (instance_layer_count == 0) {
            break;
        }

        std::vector<VkLayerProperties> tmp(instance_layer_count);
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, tmp.data());

        for (auto prop : tmp)
            g_vk_properties.push_back(prop);
    } while (res == VK_INCOMPLETE);

    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = g_vk_properties[i];
        res = init_global_extension_properties(layer_props);
        if (res != VK_SUCCESS)
            return false;
    }

    g_vk_instance_exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    g_vk_instance_exts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    VkApplicationInfo       app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;  // sType is mandatory
    app_info.pNext = NULL;                                // pNext is mandatory
    app_info.pApplicationName = "EmptyWindow";            // The name of our application
    app_info.pEngineName = NULL;                          // The name of the engine (e.g: Game engine name)
    app_info.engineVersion = 1;                           // The version of the engine
    app_info.apiVersion = VK_API_VERSION_1_0;             // The version of Vulkan we're using for this application

    VkInstanceCreateInfo    instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = (uint32_t)g_vk_instance_exts.size();
    instance_info.ppEnabledExtensionNames = g_vk_instance_exts.empty() ? nullptr : g_vk_instance_exts.data();
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = NULL;

    // create the vulkan instance
    res = vkCreateInstance(&instance_info, NULL, &g_instance);
    if (res != VK_SUCCESS) {
        MessageBox(NULL, L"Failed to create vulkan instance.", L"Error", MB_OK);
        return false;
    }

    // get the number of vulkan compatible devices
    uint32_t gpu_count = 1;
    res = vkEnumeratePhysicalDevices(g_instance, &gpu_count, NULL);
    if (res != VK_SUCCESS) {
        MessageBox(NULL, L"Failed to enumerate vulkan compatible gpu devices.", L"Error", MB_OK);
        return false;
    }
    if (gpu_count == 0) {
        MessageBox(NULL, L"Failed to find any vulkan compatible device.", L"Error", MB_OK);
        return false;
    }

    // with the correct number of vulkan compatible devices, get all detail information of these devices.
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    res = vkEnumeratePhysicalDevices(g_instance, &gpu_count, gpus.data());
    if (res != VK_SUCCESS) {
        MessageBox(NULL, L"Failed to enumerate vulkan compatible gpu devices.", L"Error", MB_OK);
        return false;
    }
    if (gpu_count == 0) {
        MessageBox(NULL, L"Failed to find any vulkan compatible device.", L"Error", MB_OK);
        return false;
    }

    VkDeviceQueueCreateInfo queue_info = {};

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, NULL);
    if (queue_family_count == 0)
        return false;

    std::vector<VkQueueFamilyProperties> queue_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, queue_props.data());

    bool found = false;
    for (unsigned int i = 0; i < queue_family_count; i++) {
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_info.queueFamilyIndex = i;
            found = true;
            break;
        }
    }

    float queue_priorities[1] = { 0.0 };
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;

    g_vk_device_exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = (uint32_t)g_vk_device_exts.size();
    device_info.ppEnabledExtensionNames = g_vk_device_exts.empty() ? nullptr : g_vk_device_exts.data();
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = NULL;
    device_info.pEnabledFeatures = NULL;

    // create vulkan device
    res = vkCreateDevice(gpus[0], &device_info, NULL, &g_device);
    if (res != VK_SUCCESS) {
        MessageBox(NULL, L"Failed to create vulkan device.", L"Error", MB_OK);
        return false;
    }

    /* Create a command pool to allocate our command buffer from */
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = queue_info.queueFamilyIndex;
    cmd_pool_info.flags = 0;

    res = vkCreateCommandPool(g_device, &cmd_pool_info, NULL, &g_commnad_pool);
    if (res != VK_SUCCESS)
        return false;

    /* Create the command buffer from the command pool */
    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = NULL;
    cmd.commandPool = g_commnad_pool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd.commandBufferCount = 1;

    res = vkAllocateCommandBuffers(g_device, &cmd, &g_command);
    if (res != VK_SUCCESS)
        return false;

    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.hinstance = hInstnace;
    createInfo.hwnd = hwnd;
    res = vkCreateWin32SurfaceKHR(g_instance, &createInfo, NULL, &g_surface);
    if (res != VK_SUCCESS)
        return false;

    // Iterate over each queue to learn whether it supports presenting:
    auto pSupportsPresent = std::make_unique<VkBool32[]>(queue_family_count);
    for (uint32_t i = 0; i < queue_family_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpus[0], i, g_surface, &pSupportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    auto graphics_queue_family_index = UINT32_MAX;
    auto present_queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphics_queue_family_index == UINT32_MAX) 
                graphics_queue_family_index = i;

            if (pSupportsPresent[i] == VK_TRUE) {
                graphics_queue_family_index = i;
                present_queue_family_index = i;
                break;
            }
        }
    }

    if (present_queue_family_index == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (size_t i = 0; i < queue_family_count; ++i)
            if (pSupportsPresent[i] == VK_TRUE) {
                present_queue_family_index = (unsigned int)i;
                break;
            }
    }

    // Generate error if could not find queues that support graphics and present
    if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX) {
        MessageBox(NULL, L"Could not find a queues for graphics and present.", L"Error", MB_OK);
        return false;
    }

    // Get the list of VkFormats that are supported:
    uint32_t formatCount;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[0], g_surface, &formatCount, NULL);
    if (res != VK_SUCCESS)
        return false;
    auto surfFormats = std::make_unique<VkSurfaceFormatKHR[]>(formatCount);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[0], g_surface, &formatCount, surfFormats.get());
    if (res != VK_SUCCESS)
        return false;
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        g_back_buffer_format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else {
        if (formatCount < 1)
            return false;
        g_back_buffer_format = surfFormats[0].format;
    }
    surfFormats = nullptr;

    VkSurfaceCapabilitiesKHR surfCapabilities;
    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpus[0], g_surface, &surfCapabilities);
    if (res != VK_SUCCESS)
        return false;

    uint32_t presentModeCount;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[0], g_surface, &presentModeCount, NULL);
    if (res != VK_SUCCESS)
        return false;

    auto presentModes = std::make_unique<VkPresentModeKHR[]>(presentModeCount);
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[0], g_surface, &presentModeCount, presentModes.get());
    if (res != VK_SUCCESS)
        return false;

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {

        // get the client size of the window
        ::RECT rect;
        ::GetWindowRect(hwnd, &rect);

        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = rect.right - rect.left;
        swapchainExtent.height = rect.bottom - rect.top;
        if (swapchainExtent.width < surfCapabilities.minImageExtent.width) {
            swapchainExtent.width = surfCapabilities.minImageExtent.width;
        }
        else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width) {
            swapchainExtent.width = surfCapabilities.maxImageExtent.width;
        }

        if (swapchainExtent.height < surfCapabilities.minImageExtent.height) {
            swapchainExtent.height = surfCapabilities.minImageExtent.height;
        }
        else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height) {
            swapchainExtent.height = surfCapabilities.maxImageExtent.height;
        }
    }
    else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    uint32_t desiredNumberOfSwapChainImages = surfCapabilities.minImageCount;

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        preTransform = surfCapabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0; i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]); i++) {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = NULL;
    swapchain_ci.surface = g_surface;
    swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
    swapchain_ci.imageFormat = g_back_buffer_format;
    swapchain_ci.imageExtent.width = swapchainExtent.width;
    swapchain_ci.imageExtent.height = swapchainExtent.height;
    swapchain_ci.preTransform = preTransform;
    swapchain_ci.compositeAlpha = compositeAlpha;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.presentMode = swapchainPresentMode;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
    swapchain_ci.clipped = true;
    swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = NULL;
    uint32_t queueFamilyIndices[2] = { (uint32_t)graphics_queue_family_index, (uint32_t)present_queue_family_index };
    if (graphics_queue_family_index != present_queue_family_index) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between
        // the queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = 2;
        swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
    }

    res = vkCreateSwapchainKHR(g_device, &swapchain_ci, NULL, &g_swapchain);
    if (res != VK_SUCCESS)
        return false;

    res = vkGetSwapchainImagesKHR(g_device, g_swapchain, &g_swapchain_image_count, NULL);
    if (res != VK_SUCCESS || g_swapchain_image_count == 0)
        return false;

    auto swapchainImages = std::make_unique<VkImage[]>(g_swapchain_image_count);
    res = vkGetSwapchainImagesKHR(g_device, g_swapchain, &g_swapchain_image_count, swapchainImages.get());
    if (res != VK_SUCCESS)
        return false;

    g_swapchain_buffers.resize(g_swapchain_image_count);
    for (uint32_t i = 0; i < g_swapchain_image_count; i++) {
        g_swapchain_buffers[i].image = swapchainImages[i];
    }
    swapchainImages = nullptr;

    for (uint32_t i = 0; i < g_swapchain_image_count; i++) {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = NULL;
        color_image_view.flags = 0;
        color_image_view.image = g_swapchain_buffers[i].image;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.format = g_back_buffer_format;
        color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
        color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
        color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
        color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.layerCount = 1;

        res = vkCreateImageView(g_device, &color_image_view, NULL, &g_swapchain_buffers[i].view);
        if (res != VK_SUCCESS)
            return false;
    }

    return true;
}

void VulkanGraphicsSample::render_frame() {

}

void VulkanGraphicsSample::shutdown() {
    for (uint32_t i = 0; i < g_swapchain_image_count; i++)
        vkDestroyImageView(g_device, g_swapchain_buffers[i].view, NULL);
    vkDestroySwapchainKHR(g_device, g_swapchain, NULL);

    // destroy command list and command pool
    VkCommandBuffer cmd_bufs[1] = { g_command };
    vkFreeCommandBuffers(g_device, g_commnad_pool, 1, cmd_bufs);
    vkDestroyCommandPool(g_device, g_commnad_pool, NULL);

    // destroy vulkan device
    vkDestroyDevice(g_device, nullptr);

    // destroy the vulkan instance
    vkDestroyInstance(g_instance, nullptr);
}