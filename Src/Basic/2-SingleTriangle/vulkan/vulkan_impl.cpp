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

/*
    This tutorial demonstrate how to draw a single triangle on screen.
*/

// Allow a maximum of three outstanding presentation operations.
#define NUM_FRAMES 3

#define VERIFY(ret)         if(ret != vk::Result::eSuccess) return false;

// Vulkan instance
// A vulkan application should have one vulkan instance. Vulkan instance is used to create vulkan surface and physical devices.
vk::Instance                                    g_vk_instance;
// Vulkan compatible GPUs
// This will be implicitly destroyed after VkInstance is destroyed, so there is no need to explicitly destroy this variable.
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
vk::PhysicalDevice                              g_vk_physical_device;
// Vulkan device
// This is the abstract device concept of graphics hardware. It is responsible for creating varies kinds of resources. Note, a vulkan
// device is neither for issuing draw calls, nor for submitting command buffers.
vk::Device                                      g_vk_device;
// Vulkan graphics queue and present queue
// Unlike the EmptyWindow tutorial, which handles the corner case that graphics queue doesn't support present.
// This tutorial will assume the graphics queue always support present so that lots of logic can be simplified by a lot.
vk::Queue                                       g_vk_graphics_queue;
// Vulkan swapchain surface
vk::SurfaceKHR                                  g_vk_surface;
// Vulkan swap chain
vk::SwapchainKHR                                g_vk_swapchain;
// Vulkan image in swapchain
// Upon creation of vulkan swapchain, there are already a few images inside. This is just to explictly keep track of them.
vk::Image                                       g_vk_images[NUM_FRAMES];
// Vulkan command pool
// Command pool keeps track of all memory for command buffers.
vk::CommandPool                                 g_vk_graphics_cmd_pool;
// Vulkan command list
// In this tutorial, nothing, but clearing the backbuffer is done in this command list.
vk::CommandBuffer                               g_vk_graphics_cmd[NUM_FRAMES];
// Pipeline layout
// Description of vertex buffer layout.
vk::PipelineLayout                              g_vk_pipeline_layout;
// Pipeline cache
vk::PipelineCache                               g_vk_pipeline_cache;
// Pipeline state
vk::Pipeline                                    g_vk_pipeline;
// Vulkan fences
// Fence objects are for CPU to wait for certain operations on GPU to be done. We can write a fence on command buffer to indicate the
// previous operations are all done. CPU can choose to wait for fence to make sure the commands of its interest are already executed on
// GPU. This is a useful data structure to make sure CPU is not too fast than GPU, in this tutorial, too fast means 3 frames faster than
// GPU execution.
vk::Fence                                       g_vk_fence[NUM_FRAMES];
// Vulkan semaphores
// Different from fence, semaphores are used to explicitly sychronize between different command buffers in gpu command queues.
// One typical usage of this semaphore concepts is we need to acquire the next avaiable image in swapchain and the command buffer should 
// not start executing on command queue before it acquires the image successfully.
vk::Semaphore                                   g_vk_image_acquired_semaphores[NUM_FRAMES];
vk::Semaphore                                   g_vk_draw_complete_semaphores[NUM_FRAMES];
vk::Semaphore                                   g_vk_image_ownership_semaphores[NUM_FRAMES];

// Vulkan extensions
std::vector<const char*>                        g_device_exts;
// Vulkan instance layer
std::vector<const char*>                        g_instance_layers;
// Vulkan queue index
unsigned int                                    g_graphics_queue_family_index = UINT32_MAX;
// Current frame index
unsigned int                                    g_frame_index = 0;

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


/*
 * Create vulkan instance.
 */
static bool create_vk_instance() {
    // Vulkan instance extensions
    std::vector<const char*> instance_exts;

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
                    instance_exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
                }

                if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                    platform_surface_ext_found = 1;
                    instance_exts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
                }
            }
        }

        // if one of them is not found, it can't run vulkan
        if (!surface_ext_found || !platform_surface_ext_found)
            return false;
    }

    // create the vulkan instance
    {
        // The app information is mostly like to be ignored by the driver.
        // But in some cases, vendor specific driver may have some optimizations for certain apps, 
        // this is a great place for them to indicate the driver this is the specific app to be optimized.
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
            .setEnabledExtensionCount((uint32_t)instance_exts.size())
            .setPpEnabledExtensionNames(instance_exts.data());

        auto result = vk::createInstance(&inst_info, nullptr, &g_vk_instance);
        VERIFY(result);
    }

    return true;
}


/*
 * Create vulkan physical device.
 */
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

    return true;
}


/*
 * Create vulkan device.
 */
static bool create_vk_device(const HINSTANCE hInstnace, const HWND hwnd) {
    // Create vulkan surface
    auto const createInfo = vk::Win32SurfaceCreateInfoKHR().setHinstance(hInstnace).setHwnd(hwnd);
    auto result = g_vk_instance.createWin32SurfaceKHR(&createInfo, nullptr, &g_vk_surface);
    VERIFY(result);

    // Graphics hardware may have multiple type of queues, this will keep track of how many type of queues are available on the graphics card.
    uint32_t queue_family_count = 0;
    g_vk_physical_device.getQueueFamilyProperties(&queue_family_count, static_cast<vk::QueueFamilyProperties*>(nullptr));

    // Get all queues' properties
    auto vk_queue_properties = std::make_unique<vk::QueueFamilyProperties[]>(queue_family_count);
    g_vk_physical_device.getQueueFamilyProperties(&queue_family_count, vk_queue_properties.get());

    // Iterate over each queue to learn whether it supports presenting:
    auto supportsPresent = std::make_unique<vk::Bool32[]>(queue_family_count);
    for (uint32_t i = 0; i < queue_family_count; i++)
        g_vk_physical_device.getSurfaceSupportKHR(i, g_vk_surface, &supportsPresent[i]);

    // Pick a graphics queue and set the present queue too if the graphics queue also supports present
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (vk_queue_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            if (g_graphics_queue_family_index == UINT32_MAX) {
                g_graphics_queue_family_index = i;
            }

            assert(supportsPresent[i] == VK_TRUE);
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (g_graphics_queue_family_index == UINT32_MAX)
        return false;

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

        auto result = g_vk_physical_device.createDevice(&deviceInfo, nullptr, &g_vk_device);
        VERIFY(result);
    }

    return true;
}


/*
 * Acquire the vulkan command queues
 */
static bool acquire_vk_command_queue() {
    g_vk_device.getQueue(g_graphics_queue_family_index, 0, &g_vk_graphics_queue);
    return true;
}

/*
 * Create the vulkan swapchain.
 * This function also creates a few sychronization objects too.
 */
static bool create_vk_swapchain() {
    // Get the list of VkFormat's that are supported:
    uint32_t format_count;
    auto result = g_vk_physical_device.getSurfaceFormatsKHR(g_vk_surface, &format_count, static_cast<vk::SurfaceFormatKHR*>(nullptr));
    VERIFY(result);

    auto surface_format = std::make_unique<vk::SurfaceFormatKHR[]>(format_count);
    result = g_vk_physical_device.getSurfaceFormatsKHR(g_vk_surface, &format_count, surface_format.get());
    VERIFY(result);

    vk::Format vk_surface_format;
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (format_count == 1 && surface_format[0].format == vk::Format::eUndefined)
        vk_surface_format = vk::Format::eB8G8R8A8Unorm;
    else
        vk_surface_format = surface_format[0].format;
    auto vk_color_space = surface_format[0].colorSpace;

    // Check the surface capabilities and formats
    vk::SurfaceCapabilitiesKHR surf_caps;
    result = g_vk_physical_device.getSurfaceCapabilitiesKHR(g_vk_surface, &surf_caps);
    VERIFY(result);

    uint32_t present_mode_count;
    result = g_vk_physical_device.getSurfacePresentModesKHR(g_vk_surface, &present_mode_count, static_cast<vk::PresentModeKHR*>(nullptr));
    VERIFY(result);

    auto present_modes = std::make_unique<vk::PresentModeKHR[]>(present_mode_count);
    result = g_vk_physical_device.getSurfacePresentModesKHR(g_vk_surface, &present_mode_count, present_modes.get());
    VERIFY(result);

    vk::Extent2D swapchainExtent;
    swapchainExtent = surf_caps.currentExtent;

    vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;

    assert(NUM_FRAMES <= surf_caps.maxImageCount);

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
        .setMinImageCount(NUM_FRAMES)
        .setImageFormat(vk_surface_format)
        .setImageColorSpace(vk_color_space)
        .setImageExtent({ swapchainExtent.width, swapchainExtent.height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setQueueFamilyIndexCount(0)
        .setPQueueFamilyIndices(nullptr)
        .setPreTransform(pre_transform)
        .setCompositeAlpha(compositeAlpha)
        .setPresentMode(present_mode)
        .setClipped(true);

    result = g_vk_device.createSwapchainKHR(&swapchain_ci, nullptr, &g_vk_swapchain);
    VERIFY(result);

    return true;
}


/*
 * Acquire the images in the swapchain.
 */
static bool acquire_vk_images() {
    // check how many images there are in the swapchain
    uint32_t swapchain_image_cnt = 0;
    auto result = g_vk_device.getSwapchainImagesKHR(g_vk_swapchain, &swapchain_image_cnt, static_cast<vk::Image*>(nullptr));
    VERIFY(result);
    assert(swapchain_image_cnt == NUM_FRAMES);

    // get vulkan images in the swapchain
    result = g_vk_device.getSwapchainImagesKHR(g_vk_swapchain, &swapchain_image_cnt, g_vk_images);
    VERIFY(result);

    return true;
}


/*
 * Create sychronization objects.
 */
static bool create_vk_sychronization_objs() {
    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    auto const semaphoreCreateInfo = vk::SemaphoreCreateInfo();

    auto result = vk::Result::eSuccess;

    // Create fences that we can use to throttle if we get too far ahead of the image presents
    auto const fence_ci = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
    for (uint32_t i = 0; i < NUM_FRAMES; i++) {
        result = g_vk_device.createFence(&fence_ci, nullptr, &g_vk_fence[i]);
        VERIFY(result);

        result = g_vk_device.createSemaphore(&semaphoreCreateInfo, nullptr, &g_vk_image_acquired_semaphores[i]);
        VERIFY(result);

        result = g_vk_device.createSemaphore(&semaphoreCreateInfo, nullptr, &g_vk_draw_complete_semaphores[i]);
        VERIFY(result);
    }

    return true;
}


/*
 * Create command pool and command buffers
 */
static bool create_vk_command() {
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
    for (uint32_t i = 0; i < NUM_FRAMES; ++i) {
        result = g_vk_device.allocateCommandBuffers(&cmd, &g_vk_graphics_cmd[i]);
        VERIFY(result);
    }

    return true;
}


/*
 * Resource transition.
 */
template<vk::ImageLayout old_layout, vk::ImageLayout new_layout>
void image_transition(vk::CommandBuffer& cb, const unsigned int current_buffer, const uint32_t src_queue_index, const uint32_t dst_queue_index) {
    auto const barrier =
        vk::ImageMemoryBarrier()
        .setSrcAccessMask(vk::AccessFlags())
        .setDstAccessMask(vk::AccessFlags())
        .setOldLayout(old_layout)
        .setNewLayout(new_layout)
        .setSrcQueueFamilyIndex(src_queue_index)
        .setDstQueueFamilyIndex(dst_queue_index)
        .setImage(g_vk_images[current_buffer])
        .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    cb.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::DependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &barrier);
}


/*
 * Create graphics pipeline.
 */
static bool create_gp() {
#if 0
    // wait for shader to be ready
    vk::PipelineCacheCreateInfo const pipeline_cache_info;
    auto result = g_vk_device.createPipelineCache(&pipeline_cache_info, nullptr, &g_vk_pipeline_cache);
    VERIFY(result);

    // descriptor layout
    auto const pipeline_layout_create_info = vk::PipelineLayoutCreateInfo().setSetLayoutCount(0);
    result = g_vk_device.createPipelineLayout(&pipeline_layout_create_info, nullptr, &g_vk_pipeline_layout);
    VERIFY(result);

    // vertex format layout
    vk::VertexInputAttributeDescription vertex_input_attr_descs[] = {
        vk::VertexInputAttributeDescription(),
        vk::VertexInputAttributeDescription()
    };
    auto const vertex_input_layout = vk::PipelineVertexInputStateCreateInfo()
                                .setVertexAttributeDescriptionCount(2)
                                .setPVertexAttributeDescriptions(vertex_input_attr_descs);
    
    // input assembly info
    auto const input_assembler_info = vk::PipelineInputAssemblyStateCreateInfo()
                                .setTopology(vk::PrimitiveTopology::eTriangleList);

    // rasterizer information
    auto const rasterizer_info = vk::PipelineRasterizationStateCreateInfo()
                                .setDepthClampEnable(VK_FALSE)
                                .setRasterizerDiscardEnable(VK_FALSE)
                                .setPolygonMode(vk::PolygonMode::eFill)
                                .setCullMode(vk::CullModeFlagBits::eNone)
                                .setFrontFace(vk::FrontFace::eCounterClockwise)
                                .setDepthBiasEnable(VK_FALSE);

    // depth stencil info
    auto const depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo()
                                .setDepthTestEnable(VK_FALSE)
                                .setDepthWriteEnable(VK_FALSE);
    
    // color blend state
    vk::PipelineColorBlendAttachmentState const color_blend[1] = {
        vk::PipelineColorBlendAttachmentState().setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                                  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA) };
    auto const color_blend_state =
        vk::PipelineColorBlendStateCreateInfo().setAttachmentCount(1).setPAttachments(color_blend);

    // dynamic state
    vk::DynamicState const dynamic_states[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    auto const dynamic_state_info = vk::PipelineDynamicStateCreateInfo().setPDynamicStates(dynamic_states).setDynamicStateCount(2);

    // multi sample info
    auto const multi_sample_info = vk::PipelineMultisampleStateCreateInfo();

    // viewport info
    auto const viewport_info = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

    // pipeline create info
    auto const pipeline = vk::GraphicsPipelineCreateInfo()
                                .setStageCount(2)
                                .setPVertexInputState(&vertex_input_layout)
                                .setPInputAssemblyState(&input_assembler_info)
                                .setPRasterizationState(&rasterizer_info)
                                .setPDepthStencilState(&depth_stencil_info)
                                .setPViewportState(&viewport_info)
                                // .setPStages(shaderStageInfo) to be done
                                .setPColorBlendState(&color_blend_state)
                                .setLayout(g_vk_pipeline_layout)
                                .setPDynamicState(&dynamic_state_info)
                                .setPMultisampleState(&multi_sample_info);

    result = g_vk_device.createGraphicsPipelines(g_vk_pipeline_cache, 1, &pipeline, nullptr, &g_vk_pipeline);
    VERIFY(result);

#endif

    return true;
}

/*
 * Initialize the vulkan sample.
 * Followings are the basic steps to initialize a Vulkan application.
 *   - [Opt] Enable Vulkan validataion ( This is only active on debug build. )
 *   - Create Vulkan instance
 *   - Enumerate vulkan physical devices and use the first one
 *   - Create Vulkan surface
 *   - Enumerate all available queues and pick a graphics queue and it needs to support present too
 *   - Create Vulkan swapchain
 *     - Get all vulkan images in the swapchain
 *   - Create command pool and command buffers
 */
bool VulkanGraphicsSample::initialize(const HINSTANCE hInstnace, const HWND hwnd) {
    // enable gpu validation if needed
    enable_gpu_validation();

    // initialize vulkan instance
    if (!create_vk_instance())
        return false;

    // enumerate all d3d12 compatible graphis hardware on this machine.
    if (!create_vk_physical_device())
        return false;

    // create vulkan device
    if (!create_vk_device(hInstnace, hwnd))
        return false;

    // enumerate the vulkan command queues, this won't fail, but just to keep consistency, still checking.
    if (!acquire_vk_command_queue())
        return false;

    // create swap chain
    if (!create_vk_swapchain())
        return false;

    // acquire the images in the swapchain
    if (!acquire_vk_images())
        return false;

    // create command pool and commnad list
    if (!create_vk_command())
        return false;

    // create fence and semaphores
    if (!create_vk_sychronization_objs())
        return false;

    // create graphics pipeline
    if (!create_gp())
        return false;

    return true;
}


/*
 * Renders a frame.
 */
void VulkanGraphicsSample::render_frame() {
    static std::vector<bool> first_time(NUM_FRAMES, true);

    // making sure the frame to be written is not pending on execution
    g_vk_device.waitForFences(1, &g_vk_fence[g_frame_index], VK_TRUE, UINT64_MAX);
    g_vk_device.resetFences({ g_vk_fence[g_frame_index] });

    // Different from the frame index, which is modulated by NUM_FRAMES, this index is indicating the frame buffer index to render on.
    uint32_t current_buffer = 0;

    vk::Result result;
    result = g_vk_device.acquireNextImageKHR(g_vk_swapchain, UINT64_MAX, g_vk_image_acquired_semaphores[g_frame_index], vk::Fence(), &current_buffer);
    assert(result == vk::Result::eSuccess);

    // generate the command buffer
    {
        auto const commandInfo = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        g_vk_graphics_cmd[g_frame_index].reset((vk::CommandBufferResetFlags)0);

        auto result = g_vk_graphics_cmd[g_frame_index].begin(&commandInfo);

        if (first_time[current_buffer]) {
            image_transition<vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal>(g_vk_graphics_cmd[g_frame_index], current_buffer, g_graphics_queue_family_index, g_graphics_queue_family_index);
            first_time[current_buffer] = false;
        }
        else {
            image_transition<vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eTransferDstOptimal>(g_vk_graphics_cmd[g_frame_index], current_buffer, g_graphics_queue_family_index, g_graphics_queue_family_index);
        }

        // clear the back buffer
        g_vk_graphics_cmd[g_frame_index].clearColorImage(g_vk_images[g_frame_index], vk::ImageLayout::eTransferDstOptimal,
            vk::ClearColorValue(std::array<float, 4>({ {0.4f, 0.6f, 1.0f, 1.0f} })),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        image_transition<vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR>(g_vk_graphics_cmd[g_frame_index], current_buffer, g_graphics_queue_family_index, g_graphics_queue_family_index);

        g_vk_graphics_cmd[g_frame_index].end();
    }

    vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo()
        .setPWaitDstStageMask(&pipe_stage_flags)
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&g_vk_image_acquired_semaphores[g_frame_index])
        .setCommandBufferCount(1)
        .setPCommandBuffers(&g_vk_graphics_cmd[g_frame_index])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&g_vk_draw_complete_semaphores[g_frame_index]);

    result = g_vk_graphics_queue.submit(1, &submit_info, g_vk_fence[g_frame_index]);
    assert(result == vk::Result::eSuccess);

    auto const presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&g_vk_draw_complete_semaphores[g_frame_index])
        .setSwapchainCount(1)
        .setPSwapchains(&g_vk_swapchain)
        .setPImageIndices(&current_buffer);

    result = g_vk_graphics_queue.presentKHR(&presentInfo);
    assert(result == vk::Result::eSuccess);

    g_frame_index += 1;
    g_frame_index %= NUM_FRAMES;
}


/*
 * Teardown vulkan related stuff.
 */
void VulkanGraphicsSample::shutdown() {
    // Wait for fences from present operations
    for (uint32_t i = 0; i < NUM_FRAMES; i++) {
        g_vk_device.waitForFences(1, &g_vk_fence[i], VK_TRUE, UINT64_MAX);
        g_vk_device.destroyFence(g_vk_fence[i], nullptr);
        g_vk_device.destroySemaphore(g_vk_image_acquired_semaphores[i], nullptr);
        g_vk_device.destroySemaphore(g_vk_draw_complete_semaphores[i], nullptr);
    }

    g_vk_device.destroySwapchainKHR(g_vk_swapchain, nullptr);

    for (auto& cmd : g_vk_graphics_cmd)
        g_vk_device.freeCommandBuffers(g_vk_graphics_cmd_pool, { cmd });
    g_vk_device.destroyCommandPool(g_vk_graphics_cmd_pool, nullptr);

#if 0
    g_vk_device.destroyPipeline(g_vk_pipeline);
    g_vk_device.destroyPipelineCache(g_vk_pipeline_cache);
    g_vk_device.destroyPipelineLayout(g_vk_pipeline_layout);
#endif

    g_vk_device.waitIdle();
    g_vk_device.destroy(nullptr);
    g_vk_instance.destroySurfaceKHR(g_vk_surface, nullptr);
    g_vk_instance.destroy(nullptr);
}