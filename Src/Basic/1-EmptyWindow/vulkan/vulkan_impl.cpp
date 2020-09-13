//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <vector>
#include <windows.h>
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

    return true;
}

void VulkanGraphicsSample::render_frame() {

}

void VulkanGraphicsSample::shutdown() {
    // destroy command list and command pool
    VkCommandBuffer cmd_bufs[1] = { g_command };
    vkFreeCommandBuffers(g_device, g_commnad_pool, 1, cmd_bufs);
    vkDestroyCommandPool(g_device, g_commnad_pool, NULL);

    // destroy vulkan device
    vkDestroyDevice(g_device, nullptr);

    // destroy the vulkan instance
    vkDestroyInstance(g_instance, nullptr);
}