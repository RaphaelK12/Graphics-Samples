//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <vector>
#include <windows.h>
#include <memory>
#include "vulkan_impl.h"

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2

#define VERIFY(ret)         if(ret != vk::Result::eSuccess) return false;

// Vulkan instance
vk::Instance                                    g_vk_instance;
// Vulkan compatible GPUs
vk::PhysicalDevice                              g_vk_physical_device;
// Vulkan queue properties
std::unique_ptr<vk::QueueFamilyProperties[]>    g_vk_queue_properties;
// swap chain surface
vk::SurfaceKHR                                  g_vk_surface;
// graphics queue and present queue
vk::Queue                                       g_vk_graphics_queue;
vk::Queue                                       g_vk_present_queue;
// Vulkan device
vk::Device                                      g_vk_device;
// Vulkan surface format
vk::Format                                      g_vk_surface_format;
// Vulkan color space
vk::ColorSpaceKHR                               g_vk_color_space;
// Vulkan fences
vk::Fence                                       g_vk_fence[FRAME_LAG];
// Vulkan semaphores
vk::Semaphore                                   g_vk_image_acquired_semaphores[FRAME_LAG];
vk::Semaphore                                   g_vk_draw_complete_semaphores[FRAME_LAG];
vk::Semaphore                                   g_vk_image_ownership_semaphores[FRAME_LAG];
// vulkan image in swap chain
std::vector<vk::Image>                          g_vk_images;
// vulkan image view in swap chain
std::vector<vk::ImageView>                      g_vk_image_views;
// Vulkan graphics command list
std::vector<vk::CommandBuffer>                  g_vk_graphics_cmd;
// Vulkan present command list
std::vector<vk::CommandBuffer>                  g_vk_present_cmd;
// Vulkan swap chain
vk::SwapchainKHR                                g_vk_swapchain;
// Vulkan command pool
vk::CommandPool                                 g_vk_graphics_cmd_pool;
vk::CommandPool                                 g_vk_present_cmd_pool;


// Vulkan queue family count
uint32_t                                        g_queue_family_count;
// Vulkan layer count
uint32_t                                        g_enabled_layer_count = 0;
// Vulkan extensions
std::vector<const char*>                        g_device_exts;
// Vulkan instance extensions
std::vector<const char*>                        g_instance_exts;
// Vulkan instance layer
std::vector<const char*>                        g_instance_layers;

// Whether there are separate graphics and present queue
bool                                            g_separate_queue;
// Current frame index
unsigned int                                    g_frame_index;

// Vulkan queue index
unsigned int                                    g_graphics_queue_family_index = UINT32_MAX;
unsigned int                                    g_present_queue_family_index = UINT32_MAX;

// current window size
uint32_t                                        g_width = 0;
uint32_t                                        g_height = 0;

// final swap chain image count
uint32_t                                        g_swapchain_image_cnt = 0;

/*
 * Enable gpu validation.
 */
bool enable_gpu_validation() {
#if defined(_DEBUG)
    uint32_t instance_layer_count = 0;
    auto result = vk::enumerateInstanceLayerProperties(&instance_layer_count, static_cast<vk::LayerProperties*>(nullptr));
    VERIFY(result);

    if (instance_layer_count > 0) {
        std::unique_ptr<vk::LayerProperties[]> instance_layers(new vk::LayerProperties[instance_layer_count]);
        result = vk::enumerateInstanceLayerProperties(&instance_layer_count, instance_layers.get());
        VERIFY(result);

        // check the validation layer
        {
            const char* valid_layer_str = "VK_LAYER_KHRONOS_validation";

            for (uint32_t j = 0; j < instance_layer_count; j++) {
                const char* t = instance_layers[j].layerName;
                if (!strcmp(valid_layer_str, instance_layers[j].layerName)) {
                    g_instance_layers.push_back(valid_layer_str);
                    return true;
                }
            }
        }
    }
#endif

    return false;
}

static bool create_vk_instance() {
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
                    g_instance_exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
                }

                if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                    platform_surface_ext_found = 1;
                    g_instance_exts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
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
            .setEnabledLayerCount((uint32_t)g_instance_layers.size())
            .setPpEnabledLayerNames(g_instance_layers.data())
            .setEnabledExtensionCount((uint32_t)g_instance_exts.size())
            .setPpEnabledExtensionNames(g_instance_exts.data());

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
                    g_device_exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                }
            }
        }

        if (!swapchain_ext_found)
            return false;
    }

    g_vk_physical_device.getQueueFamilyProperties(&g_queue_family_count, static_cast<vk::QueueFamilyProperties*>(nullptr));

    g_vk_queue_properties = std::make_unique<vk::QueueFamilyProperties[]>(g_queue_family_count);
    g_vk_physical_device.getQueueFamilyProperties(&g_queue_family_count, g_vk_queue_properties.get());

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
    auto supportsPresent = std::make_unique<vk::Bool32[]>(g_queue_family_count);
    for (uint32_t i = 0; i < g_queue_family_count; i++)
        g_vk_physical_device.getSurfaceSupportKHR(i, g_vk_surface, &supportsPresent[i]);

    for (uint32_t i = 0; i < g_queue_family_count; i++) {
        if (g_vk_queue_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            if (g_graphics_queue_family_index == UINT32_MAX) {
                g_graphics_queue_family_index = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                g_present_queue_family_index = i;
                g_present_queue_family_index = i;
                break;
            }
        }
    }

    if (g_present_queue_family_index == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then find a separate present queue.
        for (uint32_t i = 0; i < g_queue_family_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                g_present_queue_family_index = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (g_graphics_queue_family_index == UINT32_MAX || g_present_queue_family_index == UINT32_MAX)
        return false;

    g_separate_queue = g_graphics_queue_family_index != g_present_queue_family_index;

    // Create vulkan device
    {
        float const priorities[1] = { 0.0 };

        vk::DeviceQueueCreateInfo queues[2];
        queues[0].setQueueFamilyIndex(g_graphics_queue_family_index);
        queues[0].setQueueCount(1);
        queues[0].setPQueuePriorities(priorities);

        auto deviceInfo = vk::DeviceCreateInfo()
            .setQueueCreateInfoCount(1)
            .setPQueueCreateInfos(queues)
            .setEnabledLayerCount(0)
            .setPpEnabledLayerNames(nullptr)
            .setEnabledExtensionCount((uint32_t)g_device_exts.size())
            .setPpEnabledExtensionNames((const char* const*)g_device_exts.data())
            .setPEnabledFeatures(nullptr);

        if (g_separate_queue) {
            queues[1].setQueueFamilyIndex(g_present_queue_family_index);
            queues[1].setQueueCount(1);
            queues[1].setPQueuePriorities(priorities);
            deviceInfo.setQueueCreateInfoCount(2);
        }

        auto result = g_vk_physical_device.createDevice(&deviceInfo, nullptr, &g_vk_device);
        VERIFY(result);
    }

    g_vk_device.getQueue(g_graphics_queue_family_index, 0, &g_vk_graphics_queue);
    g_vk_device.getQueue(g_present_queue_family_index, 0, &g_vk_present_queue);

    // Get the list of VkFormat's that are supported:
    uint32_t format_count;
    auto result = g_vk_physical_device.getSurfaceFormatsKHR(g_vk_surface, &format_count, static_cast<vk::SurfaceFormatKHR*>(nullptr));
    VERIFY(result);

    auto surface_format = std::make_unique<vk::SurfaceFormatKHR[]>(format_count);
    result = g_vk_physical_device.getSurfaceFormatsKHR(g_vk_surface, &format_count, surface_format.get());
    VERIFY(result);

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (format_count == 1 && surface_format[0].format == vk::Format::eUndefined)
        g_vk_surface_format = vk::Format::eB8G8R8A8Unorm;
    else
        g_vk_surface_format = surface_format[0].format;
    g_vk_color_space = surface_format[0].colorSpace;

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    auto const semaphoreCreateInfo = vk::SemaphoreCreateInfo();

    // Create fences that we can use to throttle if we get too far ahead of the image presents
    auto const fence_ci = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
    for (uint32_t i = 0; i < FRAME_LAG; i++) {
        result = g_vk_device.createFence(&fence_ci, nullptr, &g_vk_fence[i]);
        VERIFY(result);

        result = g_vk_device.createSemaphore(&semaphoreCreateInfo, nullptr, &g_vk_image_acquired_semaphores[i]);
        VERIFY(result);

        result = g_vk_device.createSemaphore(&semaphoreCreateInfo, nullptr, &g_vk_draw_complete_semaphores[i]);
        VERIFY(result);

        if (g_separate_queue) {
            result = g_vk_device.createSemaphore(&semaphoreCreateInfo, nullptr, &g_vk_image_ownership_semaphores[i]);
            VERIFY(result);
        }
    }
    g_frame_index = 0;

    return true;
}

static bool create_buffers() {
    vk::SwapchainKHR oldSwapchain = g_vk_swapchain;

    // Check the surface capabilities and formats
    vk::SurfaceCapabilitiesKHR surf_caps;
    auto result = g_vk_physical_device.getSurfaceCapabilitiesKHR(g_vk_surface, &surf_caps);
    VERIFY(result);

    uint32_t present_mode_count;
    result = g_vk_physical_device.getSurfacePresentModesKHR(g_vk_surface, &present_mode_count, static_cast<vk::PresentModeKHR*>(nullptr));
    VERIFY(result);

    auto present_modes = std::make_unique<vk::PresentModeKHR[]>(present_mode_count);
    result = g_vk_physical_device.getSurfacePresentModesKHR(g_vk_surface, &present_mode_count, present_modes.get());
    VERIFY(result);

    vk::Extent2D swapchainExtent;
    swapchainExtent = surf_caps.currentExtent;

    g_width = surf_caps.currentExtent.width;
    g_height = surf_caps.currentExtent.height;

    vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;

    uint32_t swapchain_image_cnt = 3;
    if (surf_caps.maxImageCount > 0 && swapchain_image_cnt > surf_caps.maxImageCount)
        swapchain_image_cnt = surf_caps.maxImageCount;

    vk::SurfaceTransformFlagBitsKHR pre_transform;
    if (surf_caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        pre_transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    else
        pre_transform = surf_caps.currentTransform;

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    vk::CompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit,
    };
    for (uint32_t i = 0; i < _countof(compositeAlphaFlags); i++) {
        if (surf_caps.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    auto const swapchain_ci = vk::SwapchainCreateInfoKHR()
        .setSurface(g_vk_surface)
        .setMinImageCount(swapchain_image_cnt)
        .setImageFormat(g_vk_surface_format)
        .setImageColorSpace(g_vk_color_space)
        .setImageExtent({ swapchainExtent.width, swapchainExtent.height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setQueueFamilyIndexCount(0)
        .setPQueueFamilyIndices(nullptr)
        .setPreTransform(pre_transform)
        .setCompositeAlpha(compositeAlpha)
        .setPresentMode(present_mode)
        .setClipped(true)
        .setOldSwapchain(oldSwapchain);

    result = g_vk_device.createSwapchainKHR(&swapchain_ci, nullptr, &g_vk_swapchain);
    VERIFY(result);

    result = g_vk_device.getSwapchainImagesKHR(g_vk_swapchain, &g_swapchain_image_cnt, static_cast<vk::Image*>(nullptr));
    VERIFY(result);

    // get vulkan images in the swapchain
    g_vk_images.resize(g_swapchain_image_cnt);
    result = g_vk_device.getSwapchainImagesKHR(g_vk_swapchain, &g_swapchain_image_cnt, g_vk_images.data());
    VERIFY(result);

    // get the image views for each image in the swap chain
    g_vk_image_views.resize(g_swapchain_image_cnt);
    for (uint32_t i = 0; i < g_swapchain_image_cnt; ++i) {
        auto color_image_view = vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(g_vk_surface_format)
            .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
            .setImage(g_vk_images[i]);

        result = g_vk_device.createImageView(&color_image_view, nullptr, &g_vk_image_views[i]);
        VERIFY(result);
    }

    return true;
}

static bool create_command() {
    auto const cmd_pool_info = vk::CommandPoolCreateInfo()
        .setQueueFamilyIndex(g_graphics_queue_family_index)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    // create the graphics command pool
    auto result = g_vk_device.createCommandPool(&cmd_pool_info, nullptr, &g_vk_graphics_cmd_pool);
    VERIFY(result);

    // command buffer for graphics calls
    auto const cmd = vk::CommandBufferAllocateInfo()
        .setCommandPool(g_vk_graphics_cmd_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    // create the command buffer
    g_vk_graphics_cmd.resize(g_swapchain_image_cnt);
    for (uint32_t i = 0; i < g_swapchain_image_cnt; ++i) {
        result = g_vk_device.allocateCommandBuffers(&cmd, &g_vk_graphics_cmd[i]);
        VERIFY(result);
    }

    if (g_separate_queue) {
        auto const present_cmd_pool_info = vk::CommandPoolCreateInfo().setQueueFamilyIndex(g_present_queue_family_index);

        result = g_vk_device.createCommandPool(&present_cmd_pool_info, nullptr, &g_vk_present_cmd_pool);
        VERIFY(result);

        auto const present_cmd = vk::CommandBufferAllocateInfo()
            .setCommandPool(g_vk_present_cmd_pool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);

        for (uint32_t i = 0; i < g_swapchain_image_cnt; i++) {
            result = g_vk_device.allocateCommandBuffers(&present_cmd, &g_vk_present_cmd[i]);
            VERIFY(result);
        }
    }

    return true;
}

template<vk::ImageLayout old_layout, vk::ImageLayout new_layout>
void image_transition(const unsigned int current_buffer) {
    auto const barrier =
        vk::ImageMemoryBarrier()
        .setSrcAccessMask(vk::AccessFlags())
        .setDstAccessMask(vk::AccessFlags())
        .setOldLayout(old_layout)
        .setNewLayout(new_layout)
        .setSrcQueueFamilyIndex(g_graphics_queue_family_index)
        .setDstQueueFamilyIndex(g_present_queue_family_index)
        .setImage(g_vk_images[current_buffer])
        .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    g_vk_graphics_cmd[current_buffer].pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::DependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &barrier);
}

bool VulkanGraphicsSample::initialize(const HINSTANCE hInstnace, const HWND hwnd) {
    // enable gpu validation if needed
    enable_gpu_validation();

    // initialize vulkan instance
    if (!create_vk_instance())
        return false;

    // initialize vulkan device
    if (!create_vk_physical_device())
        return false;
    
    // create swap chain
    if (!create_swapchain(hInstnace, hwnd))
        return false;

    // create buffers
    if (!create_buffers())
        return false;

    // create command pool and commnad list
    if (!create_command())
        return false;

    return true;
}

void VulkanGraphicsSample::render_frame() {
    static std::vector<bool> first_time_transit(g_swapchain_image_cnt, true);

    uint32_t current_buffer = 0;

    g_vk_device.waitForFences(1, &g_vk_fence[g_frame_index], VK_TRUE, UINT64_MAX);
    g_vk_device.resetFences({ g_vk_fence[g_frame_index] });

    vk::Result result;
    result = g_vk_device.acquireNextImageKHR(g_vk_swapchain, UINT64_MAX, g_vk_image_acquired_semaphores[g_frame_index], vk::Fence(), &current_buffer);
    assert(result == vk::Result::eSuccess);

    // generate the command buffer
    {
        vk::DispatchLoaderStatic d;

        auto const commandInfo = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        static float g = 0.0f;
        g += 0.05f;

        g_vk_graphics_cmd[current_buffer].reset((vk::CommandBufferResetFlags)0, d);

        auto result = g_vk_graphics_cmd[current_buffer].begin(&commandInfo);

        if (first_time_transit[current_buffer]) {
            image_transition<vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal>(current_buffer);
            first_time_transit[current_buffer] = false;
        }
        else {
            image_transition<vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eTransferDstOptimal>(current_buffer);
        }

        // clear the back buffer
        g_vk_graphics_cmd[current_buffer].clearColorImage( g_vk_images[current_buffer], vk::ImageLayout::eTransferDstOptimal,
                                                        vk::ClearColorValue(std::array<float, 4>({ {0.4f, 0.6f, sinf(g), 1.0f} })),
                                                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1), d);

        image_transition<vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR>(current_buffer);

        g_vk_graphics_cmd[current_buffer].end();
    }

    vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo()
        .setPWaitDstStageMask(&pipe_stage_flags)
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&g_vk_image_acquired_semaphores[g_frame_index])
        .setCommandBufferCount(1)
        .setPCommandBuffers(&g_vk_graphics_cmd[current_buffer])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&g_vk_draw_complete_semaphores[g_frame_index]);

    result = g_vk_graphics_queue.submit(1, &submit_info, g_vk_fence[g_frame_index]);
    assert(result == vk::Result::eSuccess);

    if (g_separate_queue) {
        // If we are using separate queues, change image ownership to the
        // present queue before presenting, waiting for the draw complete
        // semaphore and signalling the ownership released semaphore when
        // finished
        auto const present_submit_info = vk::SubmitInfo()
            .setPWaitDstStageMask(&pipe_stage_flags)
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&g_vk_draw_complete_semaphores[g_frame_index])
            .setCommandBufferCount(1)
            .setPCommandBuffers(&g_vk_present_cmd[current_buffer])
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&g_vk_image_ownership_semaphores[g_frame_index]);

        result = g_vk_present_queue.submit(1, &present_submit_info, vk::Fence());
        assert(result == vk::Result::eSuccess);
    }

    auto const presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(g_separate_queue ? &g_vk_image_ownership_semaphores[g_frame_index] : &g_vk_draw_complete_semaphores[g_frame_index])
        .setSwapchainCount(1)
        .setPSwapchains(&g_vk_swapchain)
        .setPImageIndices(&current_buffer);

    result = g_vk_present_queue.presentKHR(&presentInfo);
    assert(result == vk::Result::eSuccess);

    g_frame_index += 1;
    g_frame_index %= FRAME_LAG;
}

void VulkanGraphicsSample::shutdown() {
    // wait for vulkan device to be idle
    g_vk_device.waitIdle();

    // Wait for fences from present operations
    for (uint32_t i = 0; i < FRAME_LAG; i++) {
        g_vk_device.waitForFences(1, &g_vk_fence[i], VK_TRUE, UINT64_MAX);
        g_vk_device.destroyFence(g_vk_fence[i], nullptr);
        g_vk_device.destroySemaphore(g_vk_image_acquired_semaphores[i], nullptr);
        g_vk_device.destroySemaphore(g_vk_draw_complete_semaphores[i], nullptr);
        if (g_separate_queue)
            g_vk_device.destroySemaphore(g_vk_image_ownership_semaphores[i], nullptr);
    }

    g_vk_device.destroySwapchainKHR(g_vk_swapchain, nullptr);

    for (uint32_t i = 0; i < g_swapchain_image_cnt; i++) {
        g_vk_device.destroyImageView(g_vk_image_views[i], nullptr);
        g_vk_device.freeCommandBuffers(g_vk_graphics_cmd_pool, { g_vk_graphics_cmd[i] });
    }

    g_vk_device.destroyCommandPool(g_vk_graphics_cmd_pool, nullptr);

    if (g_separate_queue) {
        g_vk_device.destroyCommandPool(g_vk_present_cmd_pool, nullptr);
    }
    g_vk_device.waitIdle();
    g_vk_device.destroy(nullptr);
    g_vk_instance.destroySurfaceKHR(g_vk_surface, nullptr);
    g_vk_instance.destroy(nullptr);
}