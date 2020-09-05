//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <math.h>
#include <d3dcompiler.h>
#include "d3d12_impl.h"

using Microsoft::WRL::ComPtr;

// Different from d3d11, which implicitly accumulate command buffers of three frames and stall CPU pipeline if it is
// too fast ( faster than three frames on CPU ), d3d12 exposes the control to programmers.
// The purpose of doing so is to avoid occasionally long CPU frame, which could result in GPU idle. The reason it is
// only 3 but not any number larger is to avoid input lag, which is not exactly a problem in this tutorial.
static constexpr unsigned NUM_FRAMES = 3;


// Followings are d3d12 related data structures

// An adapter is the abstraction of graphics hardware
static ComPtr<IDXGIAdapter>                 g_adapter = nullptr;
// A d3d12 device is responsible for things like creating resources.
static ComPtr<ID3D12Device2>                g_d3d12_device = nullptr;
// Command queue is the software abstraction of GPU hardware command queue. There are three type of command queues on
// modern graphics hardware, which is also true in d3d12, graphics queue, compute queue and copy queue.
// Only graphics queue is used in this tutorial.
static ComPtr<ID3D12CommandQueue>           g_command_queue = nullptr;
// Swap chain is the abstraction of a set of back buffers.
static ComPtr<IDXGISwapChain4>              g_swap_chain = nullptr;
// The three back buffers acquired from the swap chain.
static ComPtr<ID3D12Resource>               g_back_buffers[NUM_FRAMES] = { nullptr, nullptr, nullptr };
// Descriptor is a new concept in d3d12, descriptor is what we use to describe a resource and have them linked to graphics
// pipeline. Unlike d3d11, the memory management is explicitly, no memory allocation under the hood of API. In order to
// allocate a descriptor, we need a descriptor heap, which is responsible for keeping all descriptors memory alive.
static ComPtr<ID3D12DescriptorHeap>         g_descriptor_heap = nullptr;
// Different from d3d11, command list is an exposed data structure and it is responsible for pushing GPU commands in a 
// command buffer. A program can have multiple of command lists and have different thread pushing GPU commands to different 
// command list, this is how d3d12 greatly reduce the CPU overhead of API calls. Though, it is not demonstrated in this 
// tutorial.
static ComPtr<ID3D12GraphicsCommandList>    g_command_list = nullptr;
// A command list only translate the GPU command into commands of correct format. It doesn't keep the command buffer memory, 
// the command allocator does.
static ComPtr<ID3D12CommandAllocator>       g_command_list_allocators[NUM_FRAMES] = { nullptr, nullptr, nullptr };
// Fence object is used to make sure CPU is never too fast. In this tutorial, if it is 3 frames ahead of GPU, it will be 
// stalled.
static ComPtr<ID3D12Fence>                  g_fence = nullptr;
// The committed heap for geometry data, including vertex buffer and index buffer
static ComPtr<ID3D12Resource>               g_geometry_buffer = nullptr;
// The root signature
static ComPtr<ID3D12RootSignature>          g_root_signature = nullptr;
// Pipeline state object for the draw call
static ComPtr<ID3D12PipelineState>          g_pipeline_state_object = nullptr;

// Following are some generic data of this tutorial program.

// Sometimes if CPU is 3 frames ahead of CPU, it will be stalled. This even is for notifying CPU when the command it is waiting 
// is done.
static HANDLE                               g_fence_event;
// This keeps track of what is the current back buffer index to be rendered into.
static unsigned int                         g_current_back_buffer_index = 0;
// The size of render target descriptor, this is vendor specific.
static unsigned int                         g_rtv_size = 0;
// An ever increasing value, it keeps track what value to write to the fence when each frame rendering is done.
static UINT64                               g_fence_value = 0;
// The catched value of the three frames. It keeps track of what value we used to write to the fence in the past three frames.
static UINT64                               g_frame_fence_values[NUM_FRAMES];
// The vertex buffer view
D3D12_VERTEX_BUFFER_VIEW                    g_vertex_buffer_view;
D3D12_INDEX_BUFFER_VIEW                     g_index_buffer_view;

// windows client size
unsigned int                                g_window_width = 0;
unsigned int                                g_window_height = 0;

// Following are definition of vertices data

struct float3 {
    float x, y, z;
};

/*
 * Each vertex only has position and color in it. For simplicity, position data are already defined in NDC, no transformation is
 * needed in vertex shader anymore.
 */
struct Vertex{
    float3 position;    // clip space position
    float3 color;       // a color for each vertex
};

/*
 * There are only three vertices, it is a triangle in the middle of the screen.
 */
static Vertex g_vertices[] = {
    { {-0.35f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
    { { 0.35f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} },
    { { 0.0f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f} },
};

/*
 * Indices data of the vertex buffer.
 */
static unsigned int g_indices[] = { 0, 1, 2 };

// indices count
const unsigned int g_indices_cnt = _countof(g_indices);
const unsigned int g_vertices_cnt = _countof(g_vertices);

/*
 * This function helps locate the adapter with the largest dram.
 */
bool enum_adapter() {
    // Find a proper adapter
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    const auto hRet = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hRet))
        return false;

    SIZE_T dramSize = 0;
    ComPtr<IDXGIAdapter> hardwareAdapter;
    for (unsigned int AdapterIndex = 0; factory->EnumAdapters(AdapterIndex, &hardwareAdapter) != DXGI_ERROR_NOT_FOUND; ++AdapterIndex) {
        // We are only interested D3D12 compitable adapters
        if (hardwareAdapter) {
            DXGI_ADAPTER_DESC adapterDesc;
            auto ret = hardwareAdapter->GetDesc(&adapterDesc);
            if (FAILED(ret))
                return false;

            // Adapter with the largest DRAM will be favored.
            if (adapterDesc.DedicatedVideoMemory > dramSize) {
                dramSize = adapterDesc.DedicatedVideoMemory;
                g_adapter = hardwareAdapter;
            }
        }
    }

    // unable to find a compatible adapter?
    if (0 == dramSize) {
        MessageBox(nullptr, L"Unable to find d3d12 compatible adapter, please make sure you have a d3d12 compatible display card on your machine.", L"Error", MB_OK);
        return false;
    }

    return true;
}


/*
 * Enable gpu validation.
 */
void enable_gpu_validation() {
#if defined(_DEBUG)
    // enable GPU validation
    {
        ComPtr<ID3D12Debug> spDebugController0;
        ComPtr<ID3D12Debug1> spDebugController1;
        D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0));
        spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1));
        spDebugController1->SetEnableGPUBasedValidation(true);
    }

    // enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif
}


/*
 * Create a d3d12 device.
 */
bool create_d3d12_device() {
    // create d3d12 device
    auto ret = D3D12CreateDevice(g_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_d3d12_device));
    if (FAILED(ret)) {
        MessageBox(nullptr, L"Unable to find d3d12 compatible device, please make sure you have a d3d12 compatible display card on your machine.", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(g_d3d12_device.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID::D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID::D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID::D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        pInfoQueue->PushStorageFilter(&NewFilter);
    }
#endif

    return true;
}


/*
 * Create a graphics command queue.
 */
bool create_command_queue() {
    // create a graphcis command queue
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    const auto ret = g_d3d12_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_command_queue));
    if (FAILED(ret)) {
        MessageBox(nullptr, L"Unable to create d3d12 command queue.", L"Error", MB_OK);
        return false;
    }

    return true;
}

/*
 * Create a swap chain.
 */
bool create_swap_chain(HWND hwnd) {
    // create the swap chain
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    // get the client size of the window
    ::RECT rect;
    ::GetWindowRect(hwnd, &rect);

    auto ret = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
    if (FAILED(ret))
        return false;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = rect.right - rect.left;
    swapChainDesc.Height = rect.bottom - rect.top;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = NUM_FRAMES;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    g_window_width = swapChainDesc.Width;
    g_window_height = swapChainDesc.Height;

    ComPtr<IDXGISwapChain1> swapChain1;
    ret = dxgiFactory4->CreateSwapChainForHwnd(g_command_queue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
    if (FAILED(ret))
        return false;

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
    ret = dxgiFactory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(ret))
        return false;

    swapChain1.As(&g_swap_chain);

    // get the currnet frame back buffer index
    g_current_back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();

    return true;
}


/*
 * Create a command list and three command list allocators.
 */
bool create_commands() {
    for (auto i = 0; i < NUM_FRAMES; ++i) {
        // create command list allocator
        const auto ret = g_d3d12_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_command_list_allocators[i]));
        if (FAILED(ret))
            return false;
    }

    // create the command list
    const auto ret = g_d3d12_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_command_list_allocators[0].Get(), nullptr, IID_PPV_ARGS(&g_command_list));
    if (FAILED(ret))
        return false;
    g_command_list->Close();

    return true;
}


/*
 * Create render target views.
 */
bool create_rtvs() {
    // create descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = NUM_FRAMES;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    const auto ret = g_d3d12_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&g_descriptor_heap));
    if (FAILED(ret))
        return false;

    // descriptor size could be vendor dependent
    g_rtv_size = g_d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // create the render target view for each buffer in the swap chain.
    auto handle = g_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        const auto ret = g_swap_chain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (FAILED(ret))
            return false;

        g_d3d12_device->CreateRenderTargetView(backBuffer.Get(), nullptr, handle);
        g_back_buffers[i] = backBuffer;
        handle.ptr = handle.ptr + g_rtv_size;
    }

    return true;
}

/*
 * Helper function help to flush the command queue
 */
void flush_command_queue() {
    g_command_queue->Signal(g_fence.Get(), ++g_fence_value);
    g_fence->SetEventOnCompletion(g_fence_value, g_fence_event);
    WaitForSingleObject(g_fence_event, INFINITE);
}


/*
 * Create a fence for synchronization.
 */
bool create_fence() {
    // create a fence, this is for synchronization between cpu and gpu
    const auto ret = g_d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
    if (FAILED(ret))
        return false;

    // the fence event
    g_fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    return true;
}


/*
 * A helper utility function to make resource transition a bit easier.
 */
template<D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after>
void resource_transition(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource) {
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list->ResourceBarrier(1, &barrier);
}


/*
 * This function helps to create a geomtry buffer, vertex buffer view and index buffer view.
 */
bool create_geomtry_data() {
    constexpr size_t sizeofVertex = sizeof(Vertex);
    constexpr size_t sizeofVertices = sizeofVertex * _countof(g_vertices);
    constexpr size_t sizeofIndex = sizeof(unsigned int);
    constexpr size_t sizeofIndices = sizeofIndex * _countof(g_indices);
    constexpr size_t totalSize = sizeofVertices + sizeofIndices;

    D3D12_RESOURCE_DESC buffer_desc = {};
    buffer_desc.Alignment = 0;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    buffer_desc.Height = 1;
    buffer_desc.Width = totalSize;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc.MipLevels = 1;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.SampleDesc.Quality = 0;

    ComPtr<ID3D12Resource> upload_buffer = nullptr;
    D3D12_HEAP_PROPERTIES upload_heap_prop;
    upload_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_heap_prop.Type = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap_prop.VisibleNodeMask = 1;
    upload_heap_prop.CreationNodeMask = 1;

    if (FAILED(g_d3d12_device->CreateCommittedResource(
        &upload_heap_prop,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload_buffer)
    )))
        return false;

    D3D12_HEAP_PROPERTIES heap_prop;
    heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_prop.VisibleNodeMask = 1;
    heap_prop.CreationNodeMask = 1;

    if (FAILED(g_d3d12_device->CreateCommittedResource(
        &heap_prop,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&g_geometry_buffer)
    )))
        return false;

    // copy the data to the uploading buffer
    UINT8* pRaw = 0;
    upload_buffer->Map(0, 0, reinterpret_cast<void**>(&pRaw));
    memcpy(pRaw, g_vertices, sizeofVertices);
    memcpy(pRaw + sizeofVertices, g_indices, sizeofIndices);
    upload_buffer->Unmap(0, 0);

    g_command_list_allocators[0]->Reset();
    g_command_list->Reset(g_command_list_allocators[0].Get(), 0);

    resource_transition<D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST>(g_command_list.Get(), g_geometry_buffer.Get());
    g_command_list->CopyBufferRegion(g_geometry_buffer.Get(), 0, upload_buffer.Get(), 0, totalSize);
    resource_transition<D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER>(g_command_list.Get(), g_geometry_buffer.Get());

    g_command_list->Close();

    // execute the command list
    ID3D12CommandList* const commandLists[] = {
        g_command_list.Get()
    };
    g_command_queue->ExecuteCommandLists(1, commandLists);

    // flush the command queue
    flush_command_queue();

    // create the vertex buffer and index buffer view
    g_vertex_buffer_view = D3D12_VERTEX_BUFFER_VIEW{ g_geometry_buffer->GetGPUVirtualAddress(), sizeofVertices, sizeofVertex };
    g_index_buffer_view = D3D12_INDEX_BUFFER_VIEW{ g_geometry_buffer->GetGPUVirtualAddress() + sizeofVertices, sizeofIndices, DXGI_FORMAT_R32_UINT };

    return true;
}


/*
 * Create pipeline state objects
 */
bool create_pso() {
    // Load the compiled shader blobs first.
    ComPtr<ID3DBlob> vertex_shader_blob, pixel_shader_blob;
    auto ret = D3DReadFileToBlob(L"../Resources/Shaders/vso_1_single_triangle.cso", &vertex_shader_blob);
    if (FAILED(ret))
        return false;
    ret = D3DReadFileToBlob(L"../Resources/Shaders/pso_1_single_triangle.cso", &pixel_shader_blob);
    if (FAILED(ret))
        return false;

    // This tutorial is simple enough that no root-signature is needed
    D3D12_ROOT_SIGNATURE_DESC rootSig = { 0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
    ComPtr<ID3DBlob> blob_sig, blob_errors;
    ret = D3D12SerializeRootSignature(&rootSig, D3D_ROOT_SIGNATURE_VERSION_1, &blob_sig, &blob_errors);
    if (FAILED(ret))
        return false;
    ret = g_d3d12_device->CreateRootSignature(0, blob_sig->GetBufferPointer(), blob_sig->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&g_root_signature);
    if (FAILED(ret))
        return false;

    const D3D12_INPUT_ELEMENT_DESC input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    const UINT numInputLayoutElements = _countof(input_layout);

    // Create Pipeline State Object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psod;
    memset(&psod, 0, sizeof(psod));
    psod.pRootSignature = g_root_signature.Get();
    psod.VS.BytecodeLength = vertex_shader_blob->GetBufferSize();
    psod.VS.pShaderBytecode = vertex_shader_blob->GetBufferPointer();
    psod.PS.BytecodeLength = pixel_shader_blob->GetBufferSize();
    psod.PS.pShaderBytecode = pixel_shader_blob->GetBufferPointer();
    psod.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psod.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psod.RasterizerState.FrontCounterClockwise = true;
    psod.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psod.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psod.SampleDesc.Count = 1;
    psod.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psod.NumRenderTargets = 1;
    psod.SampleMask = UINT_MAX;
    psod.InputLayout = { input_layout, numInputLayoutElements };
    psod.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psod.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psod.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    psod.DepthStencilState.DepthEnable = false;
    psod.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    return SUCCEEDED(g_d3d12_device->CreateGraphicsPipelineState(&psod, __uuidof(ID3D12PipelineState), (void**)&g_pipeline_state_object));
}


/*
 * Initialize d3d12, this includes
 *   - pick a d3d12 compatible adapter
 *   - enable gpu validation
 *   - create d3d12 device
 *   - create a graphics command queue
 *   - create a swap chain
 *   - creata a command list and three command allocators
 *   - create a descriptor heap and setup the render target views
 *   - create a fence object for CPU and GPU synchronization
 *   - create the geometry data
 *   - create pipeline state object
 */
bool D3D12GraphicsSample::initialize(const HWND hwnd) {
    auto ret = enum_adapter();
    if (!ret)
        return false;

    enable_gpu_validation();

    ret = create_d3d12_device();
    if (!ret)
        return false;

    ret = create_command_queue();
    if (!ret)
        return false;

    ret = create_swap_chain(hwnd);
    if (!ret)
        return false;

    ret = create_commands();
    if (!ret)
        return false;

    ret = create_rtvs();
    if (!ret)
        return false;

    ret = create_fence();
    if (!ret)
        return false;

    ret = create_geomtry_data();
    if (!ret)
        return false;

    ret = create_pso();
    if (!ret)
        return false;

    return true;
}

/*
 * Render a frame.
 */
void D3D12GraphicsSample::render_frame() {
    auto commandAllocator = g_command_list_allocators[g_current_back_buffer_index];
    auto backBuffer = g_back_buffers[g_current_back_buffer_index];
    auto commandList = g_command_list;

    // reset the command list and the command allocator
    {
        commandAllocator->Reset();

        // The same command list is used again here. Since there is no memory maintained in a command list, it doesn't matter if the previous
        // command list doesn't finish its execution on GPU, as long we use a different command allocator.
        commandList->Reset(commandAllocator.Get(), nullptr);
    }

    // make sure the back buffer is in correct state
    {
        // Using Resource Barriers to Synchronize Resource States in Direct3D 12
        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
        //
        // Swap chain back buffers automatically start out in the D3D12_RESOURCE_STATE_COMMON state.
        // D3D12_RESOURCE_STATE_PRESENT is a synonym for D3D12_RESOURCE_STATE_COMMON.
        resource_transition<D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET>(commandList.Get(), backBuffer.Get());
    }

    // simply clear the back buffer
    {
        FLOAT clearColor[] = { 0.4f, 0.6f, 1.0f, 1.0f };
        D3D12_CPU_DESCRIPTOR_HANDLE rtv;
        rtv.ptr = g_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + g_current_back_buffer_index * g_rtv_size;
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

        commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    }

    // issue the draw call to draw a triangle
    {
        commandList->SetPipelineState(g_pipeline_state_object.Get());
        commandList->SetGraphicsRootSignature(g_root_signature.Get());

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &g_vertex_buffer_view);
        commandList->IASetIndexBuffer(&g_index_buffer_view);

        D3D12_VIEWPORT viewport;
        viewport.TopLeftX = viewport.TopLeftY = 0;
        viewport.Width = (float)g_window_width;
        viewport.Height = (float)g_window_height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect;
        scissor_rect.left = 0;
        scissor_rect.top = 0;
        scissor_rect.right = g_window_width;
        scissor_rect.bottom = g_window_height;
        commandList->RSSetScissorRects(1, &scissor_rect);

        commandList->DrawIndexedInstanced(g_indices_cnt, 1, 0, 0, 0);
    }

    // before the back buffer can be present again, it needs to transit back to present state.
    {
        resource_transition<D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT>(commandList.Get(), backBuffer.Get());
    }

    // the only command needed in the command list is the clear call and we are done here
    commandList->Close();

    // prepare the baked command list and submit it in the command queue
    ID3D12CommandList* const commandLists[] = {
        commandList.Get()
    };
    g_command_queue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // present the frame, async is always on.
    g_swap_chain->Present(1, 0);

    // signal the fence after this frame is done on GPU
    g_command_queue->Signal(g_fence.Get(), ++g_fence_value);
    g_frame_fence_values[g_current_back_buffer_index] = g_fence_value;

    // get the currnet frame back buffer index
    g_current_back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();
    
    // wait for the fence value if CPU is three frames ahead of GPU
    if (g_fence->GetCompletedValue() < g_frame_fence_values[g_current_back_buffer_index])
    {
        g_fence->SetEventOnCompletion(g_frame_fence_values[g_current_back_buffer_index], g_fence_event);
        ::WaitForSingleObject(g_fence_event, INFINITE);
    }
}


/*
 * Shutdown d3d12, deallocate all resources we used in rendering.
 */
void D3D12GraphicsSample::shutdown() {
    // flush the command queue to make sure nothing is left in it before releasing anything.
    flush_command_queue();

    // close the event handle
    ::CloseHandle(g_fence_event);

    // These destruction is not totally necessary. However, instead of relying on the compiler to destroy them,
    // explicitly destruction will guarantee specific order of destruction.
    g_geometry_buffer = nullptr;
    g_root_signature = nullptr;
    g_pipeline_state_object = nullptr;
    g_fence = nullptr;
    g_command_list = nullptr;
    for (auto i = 0; i < NUM_FRAMES; ++i) {
        g_command_list_allocators[i] = nullptr;
        g_back_buffers[i] = nullptr;
    }
    g_descriptor_heap = nullptr;
    g_swap_chain = nullptr;
    g_command_queue = nullptr;
    g_d3d12_device = nullptr;
    g_adapter = nullptr;
}