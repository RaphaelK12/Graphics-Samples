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



    return true;
}

void VulkanGraphicsSample::render_frame() {

}

void VulkanGraphicsSample::shutdown() {
    // destroy the vulkan instance
    vkDestroyInstance(g_instance, nullptr);
}