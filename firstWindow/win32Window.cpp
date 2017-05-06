#include "buildParam.h"
#include "platform.h"
#include "vulkanWindow.h"
#include "renderer.h"
#include <assert.h>
#include <iostream>

#if VK_USE_PLATFORM_WIN32_KHR

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    VulkanWindow *window = reinterpret_cast<VulkanWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch(iMsg)
    {
        case WM_CLOSE:
        window->close();
        break;

        case WM_SIZE:
            // Not need for now.
        break;

        default:
        break;
    }

    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

uint64_t VulkanWindow::win32ClassIdCounter = 0;

void VulkanWindow::initPlatformSpecificWindow()
{
    WNDCLASSEX wndclassex {};

    assert(surfaceSize.width > 0);
    assert(surfaceSize.height > 0);

    hInstance = GetModuleHandle(nullptr);
    className = windowName + "_" + std::to_string(win32ClassIdCounter);
    win32ClassIdCounter++;

    wndclassex.cbSize = sizeof(WNDCLASSEX);
    wndclassex.style = CS_HREDRAW | CS_VREDRAW;
    wndclassex.cbClsExtra = 0;
    wndclassex.cbWndExtra = 0;
    wndclassex.lpfnWndProc = WndProc;
    wndclassex.hInstance = hInstance;
    wndclassex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclassex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclassex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclassex.lpszClassName = className.c_str();
    wndclassex.lpszMenuName = NULL;
    wndclassex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wndclassex))
    {
        assert(1 && "Cannot register window class.\n");
        fflush(stdout);
        std::exit(EXIT_FAILURE);
    }

    DWORD styleExtra = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT windowRect = {0, 0, LONG(surfaceSize.width), LONG(surfaceSize.height)};
    AdjustWindowRectEx(&windowRect, style, FALSE, styleExtra);

    hWindow = CreateWindowEx(0,
        className.c_str(),
        windowTitle.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if(!hWindow)
    {
        assert(0 && "Cannot create window.\n");
        fflush(stdout);
        std::exit(EXIT_FAILURE);
    }

    SetWindowLongPtr(hWindow, GWLP_USERDATA, (LONG_PTR)this);
    ShowWindow(hWindow, SW_SHOW);
    SetForegroundWindow(hWindow);
    SetFocus(hWindow);
}

void VulkanWindow::destroyPlatformSpecificWindow()
{
    DestroyWindow(hWindow);
    UnregisterClass(className.c_str(), hInstance);
}

void VulkanWindow::updatePlatformSpecificWindow()
{
    MSG msg;
    if(PeekMessage(&msg, hWindow, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void VulkanWindow::initPlatformSpecificSurface()
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = nullptr;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.hinstance = hInstance;
    surfaceCreateInfo.hwnd = hWindow;

    vkCreateWin32SurfaceKHR(renderer->getVulkanInstance(), &surfaceCreateInfo, nullptr, &surface);
}

void VulkanWindow::destroyPlatformSpecificSurface()
{
    vkDestroySurfaceKHR(renderer->getVulkanInstance(), surface, nullptr);
}

#endif // VK_USE_PLATFORM_WIN32_KHR
