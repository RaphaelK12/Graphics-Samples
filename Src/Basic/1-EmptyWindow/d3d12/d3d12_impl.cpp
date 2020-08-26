//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#include <wrl.h>
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <math.h>
#include "d3d12_impl.h"

using Microsoft::WRL::ComPtr;

#define NUM_FRAMES          3

static ComPtr<IDXGIAdapter>                 g_adapter = nullptr;
static ComPtr<ID3D12Device2>                g_d3d12Device = nullptr;
static ComPtr<ID3D12CommandQueue>           g_command_queue = nullptr;
static ComPtr<IDXGISwapChain4>              g_swap_chain = nullptr;
static ComPtr<ID3D12DescriptorHeap>         g_descriptor_heap = nullptr;
static ComPtr<ID3D12Resource>               g_back_buffers[NUM_FRAMES] = { nullptr, nullptr, nullptr };
static ComPtr<ID3D12CommandAllocator>       g_command_list_allocators[NUM_FRAMES] = { nullptr, nullptr, nullptr };
static ComPtr<ID3D12GraphicsCommandList>    g_command_list = nullptr;
static ComPtr<ID3D12Fence>                  g_fence = nullptr;
static HANDLE                               g_fence_event;

static unsigned int                         g_current_back_buffer_index = 0;
static unsigned int                         g_rtv_size = 0;
static UINT64                               g_fence_value = 0;
static UINT64                               g_frame_fence_values[NUM_FRAMES];
static bool                                 g_vsync = true;

bool initialize_d3d12(const HWND hwnd) {
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

    // create d3d12 device
    auto ret = D3D12CreateDevice(g_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_d3d12Device));
    if (FAILED(ret)) {
        MessageBox(nullptr, L"Unable to find d3d12 compatible device, please make sure you have a d3d12 compatible display card on your machine.", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(g_d3d12Device.As(&pInfoQueue)))
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

    // create a graphcis command queue
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    ret = g_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_command_queue));
    if (FAILED(ret)) {
        MessageBox(nullptr, L"Unable to create d3d12 command queue.", L"Error", MB_OK);
        return false;
    }

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

    ret = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
    if (FAILED(ret))
        return false;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = rect.right - rect.left;
    swapChainDesc.Height = rect.bottom - rect.top;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 3;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ret = dxgiFactory4->CreateSwapChainForHwnd(g_command_queue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
    if (FAILED(ret))
        return false;

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
    ret = dxgiFactory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(ret))
        return false;

    swapChain1.As(&g_swap_chain);

    for (auto i = 0; i < NUM_FRAMES; ++i) {
        // create command list allocator
        ret = g_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_command_list_allocators[i]));
        if (FAILED(ret))
            return false;
    }

    // get the currnet frame back buffer index
    g_current_back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();

    // create the command list
    ret = g_d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_command_list_allocators[0].Get(), nullptr, IID_PPV_ARGS(&g_command_list));
    if (FAILED(ret))
        return false;
    g_command_list->Close();

    // create descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = NUM_FRAMES;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ret = g_d3d12Device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&g_descriptor_heap));
    if (FAILED(ret))
        return false;

    // descriptor size could be vendor dependent
    g_rtv_size = g_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // create the render target view for each buffer in the swap chain.
    auto handle = g_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        auto ret = g_swap_chain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (FAILED(ret))
            return false;

        g_d3d12Device->CreateRenderTargetView(backBuffer.Get(), nullptr, handle);
        g_back_buffers[i] = backBuffer;
        handle.ptr = handle.ptr + g_rtv_size;
    }

    // create a fence, this is for synchronization between cpu and gpu
    g_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));

    // the fence event
    g_fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    return true;
}

void render_frame() {
    auto commandAllocator = g_command_list_allocators[g_current_back_buffer_index];
    auto backBuffer = g_back_buffers[g_current_back_buffer_index];
    auto commandList = g_command_list;

    // reset the command list allocator
    {
        commandAllocator->Reset();
        // reset the command list
        commandList->Reset(commandAllocator.Get(), nullptr);
    }

    // make sure the back buffer is in correct state
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    // simply clear the back buffer
    {
        static float g = 0.0f;
        g += 0.05f;
        FLOAT clearColor[] = { 0.4f, 0.6f, sinf(g), 1.0f };
        D3D12_CPU_DESCRIPTOR_HANDLE rtv;
        rtv.ptr = g_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + g_current_back_buffer_index * g_rtv_size;
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // resource transition again
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    // the only command needed in the command list is the clear call and we are done here
    commandList->Close();

    // prepare the baked command list and submit it in the command queue
    ID3D12CommandList* const commandLists[] = {
        commandList.Get()
    };
    g_command_queue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // present the frame
    UINT syncInterval = g_vsync ? 1 : 0;
    g_swap_chain->Present(syncInterval, 0);

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

void shutdown_d3d12() {
    // make sure all commands on gpu command list are executed already on GPU
    g_command_queue->Signal(g_fence.Get(), ++g_fence_value);
    auto cur = g_fence->GetCompletedValue();
    if ( cur < g_fence_value)
    {
        g_fence->SetEventOnCompletion(g_fence_value, g_fence_event);
        ::WaitForSingleObject(g_fence_event, INFINITE);
    }

    // close the event handle
    ::CloseHandle(g_fence_event);

    g_fence = nullptr;
    g_command_list = nullptr;
    for (auto i = 0; i < NUM_FRAMES; ++i) {
        g_command_list_allocators[i] = nullptr;
        g_back_buffers[i] = nullptr;
    }
    g_descriptor_heap = nullptr;
    g_swap_chain = nullptr;
    g_command_queue = nullptr;
    g_d3d12Device = nullptr;
    g_adapter = nullptr;
}