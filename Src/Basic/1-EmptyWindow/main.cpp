//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#if PLATFORM_WIN

#include <windows.h>

// class name and window title
static constexpr wchar_t  CLASS_NAME[] = L"Jiayin's Graphics Samples";
static constexpr wchar_t  WINDOW_TITLE[] = L"1 - EmptyWindow";

// Whether the program is quitting
static bool g_quiting = false;

static inline LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        g_quiting = true;
        break;
    case WM_PAINT:
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Entry point of the application
int WINAPI WinMain(HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nShowCmd) {
    // Register the window class.
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInInstance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    wcex.hIconSm = 0;
    RegisterClassExW(&wcex);

    // Create the window.
    HWND hwnd = CreateWindowW(CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInInstance, nullptr);
    if (hwnd == NULL)
        return 0;

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);

    MSG msg;
    while (true) {
        // Handle window messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_quiting)
            break;
    }

    return 0;
}

#endif