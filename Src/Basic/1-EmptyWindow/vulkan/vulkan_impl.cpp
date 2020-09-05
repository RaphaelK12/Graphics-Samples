//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <vector>
#include <windows.h>
#include "vulkan/vulkan.h"
#include "vulkan_impl.h"

// This is the vulkan instance
VkInstance              g_instance;
// vulkan device
VkDevice                g_device;
// vulkan command pool
VkCommandPool           g_commnad_pool;
// vulkan command
VkCommandBuffer         g_command;

bool VulkanGraphicsSample::initialize(const HWND hwnd) {
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
    instance_info.enabledExtensionCount = 0;
    instance_info.ppEnabledExtensionNames = NULL;
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = NULL;

    // create the vulkan instance
    VkResult res = vkCreateInstance(&instance_info, NULL, &g_instance);
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
    device_info.enabledExtensionCount = 0;
    device_info.ppEnabledExtensionNames = NULL;
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