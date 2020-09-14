//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <vector>
#include <windows.h>
#include <memory>
#include "vulkan/vulkan.h"
#include "vulkan_impl.h"

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#define VERIFY(ret)         if(ret != vk::Result::eSuccess) return false;

// Instance validation layer
static char const* const                        g_instance_validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
// Vulkan layer count
uint32_t                                        g_enabled_layer_count = 0;
// Vulkan extensions
std::vector<const char*>                        g_vk_device_exts;
// Vulkan instance extensions
std::vector<const char*>                        g_vk_instance_exts;
// Vulkan instance
vk::Instance                                    g_vk_instance;
// Vulkan compatible GPUs
vk::PhysicalDevice                              g_vk_physical_device;
// Vulkan queue family count
uint32_t                                        g_vk_queue_family_count;
// Vulkan queue properties
std::unique_ptr<vk::QueueFamilyProperties[]>    g_vk_queue_properties;
// swap chain surface
vk::SurfaceKHR                                  g_vk_surface;
// graphics queue and present queue
vk::Queue                                       g_vk_graphics_queue;
vk::Queue                                       g_vk_present_queue;
// vulkan device
vk::Device                                      g_vk_device;

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
// frame buffers
std::vector<VkFramebuffer>  g_frame_buffers;
// current buffer index
uint32_t                g_current_buffer_index = 0;
// current window size
uint32_t                g_width = 0;
uint32_t                g_height = 0;
// fence
VkFence                 g_draw_fence;
// graphics queue and present queue
VkQueue                 g_graphics_queue;
VkQueue                 g_present_queue;
// image acquire semaphore
VkSemaphore             g_image_acquired_semaphore;

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

static bool check_layers(uint32_t check_count, char const* const* const check_names, uint32_t layer_count, vk::LayerProperties* layers) {
    for (uint32_t i = 0; i < check_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
            return false;
        }
    }
    return true;
}

static bool create_vk_instance() {
    // validate layer count
    {
        uint32_t instance_extension_count = 0;
        uint32_t instance_layer_count = 0;
        auto result = vk::enumerateInstanceLayerProperties(&instance_layer_count, static_cast<vk::LayerProperties*>(nullptr));
        VERIFY(result);

        if (instance_layer_count > 0) {
            std::unique_ptr<vk::LayerProperties[]> instance_layers(new vk::LayerProperties[instance_layer_count]);
            result = vk::enumerateInstanceLayerProperties(&instance_layer_count, instance_layers.get());
            VERIFY(result);

            const bool validation_found = check_layers(_countof(g_instance_validation_layers), g_instance_validation_layers, instance_layer_count, instance_layers.get());
            if (!validation_found)
                return false;
        }
    }

    // get vulkan properties
    {
        bool surface_ext_found = false, platform_surface_ext_found = false;

        // get the number of instance extension properties
        uint32_t    instance_extension_count = 0;
        auto result = vk::enumerateInstanceExtensionProperties(nullptr, &instance_extension_count, static_cast<vk::ExtensionProperties*>(nullptr));

        if (instance_extension_count > 0) {
            auto instance_extensions = std::make_unique<vk::ExtensionProperties[]>(instance_extension_count);
            result = vk::enumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.get());
            VERIFY(result);

            for (uint32_t i = 0; i < instance_extension_count; i++) {
                if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                    surface_ext_found = 1;
                    g_vk_instance_exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
                }

                if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                    platform_surface_ext_found = 1;
                    g_vk_instance_exts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
                }
            }
        }

        // if one of them is not found, it can't run vulkan
        if (!surface_ext_found || !platform_surface_ext_found)
            return false;
    }

    // create the vulkan instance
    {
        auto const app = vk::ApplicationInfo()
            .setPApplicationName("1 - EmptyWindow")
            .setApplicationVersion(0)
            .setPEngineName("1 - EmptyWindow")
            .setEngineVersion(0)
            .setApiVersion(VK_API_VERSION_1_0);
        auto const inst_info = vk::InstanceCreateInfo()
            .setPApplicationInfo(&app)
            .setEnabledLayerCount(g_enabled_layer_count)
            .setPpEnabledLayerNames(g_instance_validation_layers)
            .setEnabledExtensionCount((uint32_t)g_vk_instance_exts.size())
            .setPpEnabledExtensionNames(g_vk_instance_exts.data());

        auto result = vk::createInstance(&inst_info, nullptr, &g_vk_instance);
        VERIFY(result);
    }

    return true;
}

static bool create_vk_physical_device() {
    // initialize physical device
    {
        /* Make initial call to query gpu_count, then second call for gpu info*/
        uint32_t gpu_count;
        auto result = g_vk_instance.enumeratePhysicalDevices(&gpu_count, static_cast<vk::PhysicalDevice*>(nullptr));
        VERIFY(result);
        if (gpu_count == 0)
            return false;

        auto physical_devices = std::make_unique<vk::PhysicalDevice[]>(gpu_count);
        result = g_vk_instance.enumeratePhysicalDevices(&gpu_count, physical_devices.get());
        VERIFY(result);

        // just pick the first compatible device for now
        g_vk_physical_device = physical_devices[0];
    }

    {
        /* Look for device extensions */
        uint32_t device_extension_count = 0;
        bool swapchain_ext_found = false;

        auto result = g_vk_physical_device.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, static_cast<vk::ExtensionProperties*>(nullptr));
        VERIFY(result);

        if (device_extension_count > 0) {
            auto device_exts = std::make_unique<vk::ExtensionProperties[]>(device_extension_count);
            result = g_vk_physical_device.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, device_exts.get());
            VERIFY(result);

            for (uint32_t i = 0; i < device_extension_count; i++) {
                if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_exts[i].extensionName)) {
                    swapchain_ext_found = 1;
                    g_vk_device_exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                }
            }
        }

        if (!swapchain_ext_found)
            return false;
    }

    g_vk_physical_device.getQueueFamilyProperties(&g_vk_queue_family_count, static_cast<vk::QueueFamilyProperties*>(nullptr));

    g_vk_queue_properties = std::make_unique<vk::QueueFamilyProperties[]>(g_vk_queue_family_count);
    g_vk_physical_device.getQueueFamilyProperties(&g_vk_queue_family_count, g_vk_queue_properties.get());

    return true;
}

static bool create_surface(const HINSTANCE hInstnace, const HWND hwnd) {
    auto const createInfo = vk::Win32SurfaceCreateInfoKHR().setHinstance(hInstnace).setHwnd(hwnd);

    auto result = g_vk_instance.createWin32SurfaceKHR(&createInfo, nullptr, &g_vk_surface);
    VERIFY(result);

    return true;
}

static bool create_swapchain(const HINSTANCE hInstnace, const HWND hwnd) {
    if (!create_surface(hInstnace, hwnd))
        return false;

    // Iterate over each queue to learn whether it supports presenting:
    auto supportsPresent = std::make_unique<vk::Bool32[]>(g_vk_queue_family_count);
    for (uint32_t i = 0; i < g_vk_queue_family_count; i++) {
        g_vk_physical_device.getSurfaceSupportKHR(i, g_surface, &supportsPresent[i]);
    }

    uint32_t graphics_queue_family_index = UINT32_MAX;
    uint32_t present_queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < g_vk_queue_family_count; i++) {
        if (g_vk_queue_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            if (graphics_queue_family_index == UINT32_MAX) {
                graphics_queue_family_index = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                present_queue_family_index = i;
                present_queue_family_index = i;
                break;
            }
        }
    }

    if (present_queue_family_index == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then find a separate present queue.
        for (uint32_t i = 0; i < g_vk_queue_family_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                present_queue_family_index = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX)
        return false;

    // Create vulkan device
    {
        float const priorities[1] = { 0.0 };

        vk::DeviceQueueCreateInfo queues[2];
        queues[0].setQueueFamilyIndex(graphics_queue_family_index);
        queues[0].setQueueCount(1);
        queues[0].setPQueuePriorities(priorities);

        auto deviceInfo = vk::DeviceCreateInfo()
            .setQueueCreateInfoCount(1)
            .setPQueueCreateInfos(queues)
            .setEnabledLayerCount(0)
            .setPpEnabledLayerNames(nullptr)
            .setEnabledExtensionCount((uint32_t)g_vk_device_exts.size())
            .setPpEnabledExtensionNames((const char* const*)g_vk_device_exts.data())
            .setPEnabledFeatures(nullptr);

        if (graphics_queue_family_index != present_queue_family_index) {
            queues[1].setQueueFamilyIndex(present_queue_family_index);
            queues[1].setQueueCount(1);
            queues[1].setPQueuePriorities(priorities);
            deviceInfo.setQueueCreateInfoCount(2);
        }

        auto result = g_vk_physical_device.createDevice(&deviceInfo, nullptr, &g_vk_device);
        VERIFY(result);
    }

    g_vk_device.getQueue(graphics_queue_family_index, 0, &g_vk_graphics_queue);
    g_vk_device.getQueue(present_queue_family_index, 0, &g_vk_present_queue);

    return true;
}

bool VulkanGraphicsSample::initialize(const HINSTANCE hInstnace, const HWND hwnd) {
    // initialize vulkan instance
    if (!create_vk_instance())
        return false;

    // initialize vulkan device
    if (!create_vk_physical_device())
        return false;

    // create surface
    if (!create_surface(hInstnace, hwnd))
        return false;

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
    auto res = vkCreateInstance(&instance_info, NULL, &g_instance);
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

    // get the client size of the window
    ::RECT rect;
    ::GetWindowRect(hwnd, &rect);
    g_width = rect.right - rect.left;
    g_height = rect.bottom - rect.top;

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
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

    /* Need attachments for render target and depth buffer */
    VkAttachmentDescription attachments[2];
    attachments[0].format = g_back_buffer_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].flags = 0;

    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference = {};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = NULL;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    // Subpass dependency to wait for wsi image acquired semaphore before starting layout transition
    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    // Initialize frame buffers
    VkImageView image_attachments[1];

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = NULL;
    fb_info.attachmentCount = 1;
    fb_info.pAttachments = image_attachments;
    fb_info.width = swapchainExtent.width;
    fb_info.height = swapchainExtent.height;
    fb_info.layers = 1;

    g_frame_buffers.resize(g_swapchain_image_count);

    for (uint32_t i = 0; i < g_swapchain_image_count; i++) {
        image_attachments[0] = g_swapchain_buffers[i].view;
        res = vkCreateFramebuffer(g_device, &fb_info, NULL, &g_frame_buffers[i]);
        if (res != VK_SUCCESS)
            return false;
    }

    vkGetDeviceQueue(g_device, graphics_queue_family_index, 0, &g_graphics_queue);
    if (graphics_queue_family_index == present_queue_family_index)
        g_present_queue = g_graphics_queue;
    else
        vkGetDeviceQueue(g_device, present_queue_family_index, 0, &g_present_queue);

    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(g_device, &fenceInfo, NULL, &g_draw_fence);

    VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
    imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    imageAcquiredSemaphoreCreateInfo.pNext = NULL;
    imageAcquiredSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(g_device, &imageAcquiredSemaphoreCreateInfo, NULL, &g_image_acquired_semaphore);

    return true;
}

void VulkanGraphicsSample::render_frame() {
    // Get the index of the next available swapchain image:
    vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX, g_image_acquired_semaphore, VK_NULL_HANDLE, &g_current_buffer_index);

    // generate the command list
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(g_command, &beginInfo);

        VkImageSubresourceRange imageRange = {};
        imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageRange.levelCount = 1;
        imageRange.layerCount = 1;

        static float g = 0.0f;
        g += 0.05f;
        VkClearColorValue clearColor = { 0.4f, 0.6f, sinf(g), 1.0f };
        vkCmdClearColorImage(g_command, g_swapchain_buffers[g_current_buffer_index].image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);

        vkEndCommandBuffer(g_command);
    }

    // execute the command list
    {
        const VkCommandBuffer cmd_bufs[] = { g_command };

        VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info[1] = {};
        submit_info[0].pNext = NULL;
        submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[0].waitSemaphoreCount = 1;
        submit_info[0].pWaitSemaphores = &g_image_acquired_semaphore;
        submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
        submit_info[0].commandBufferCount = 1;
        submit_info[0].pCommandBuffers = cmd_bufs;
        submit_info[0].signalSemaphoreCount = 0;
        submit_info[0].pSignalSemaphores = NULL;

        /* Queue the command buffer for execution */
        vkQueueSubmit(g_graphics_queue, 1, submit_info, g_draw_fence);
    }

    // present the frame
    {
        VkPresentInfoKHR present;
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.pNext = NULL;
        present.swapchainCount = 1;
        present.pSwapchains = &g_swapchain;
        present.pImageIndices = &g_current_buffer_index;
        present.pWaitSemaphores = NULL;
        present.waitSemaphoreCount = 0;
        present.pResults = NULL;

        vkQueuePresentKHR(g_present_queue, &present);
    }
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