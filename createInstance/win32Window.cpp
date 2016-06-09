#include "buildParam.h"
#include "platform.h"
#include "VulkanWindow.h"
#include <assert.h>

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

    assert(surfaceSizeX > 0);
    assert(surfaceSizeY > 0);

    instance = GetModuleHandle(nullptr);
    className = windowName + "_" + std::to_string(win32ClassIdCounter);
    win32ClassIdCounter++;

    wndclassex.cbSize = sizeof(WNDCLASSEX);
    wndclassex.style = CS_HREDRAW | CS_VREDRAW;
    wndclassex.cbClsExtra = 0;
    wndclassex.cbWndExtra = 0;
    wndclassex.lpfnWndProc = WndProc;
    wndclassex.hInstance = instance;
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
        std::exit(-1);
    }

    DWORD styleExtra = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT windowRect = {0, 0, LONG(surfaceSizeX), LONG(surfaceSizeY)};
    AdjustWindowRectEx(&windowRect, style, FALSE, styleExtra);

    window = CreateWindowEx(0,
        className.c_str(),
        windowName.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        instance,
        NULL);

    if(!window)
    {
        assert(0 && "Cannot create window.\n");
        fflush(stdout);
        std::exit(-1);
    }

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)this);
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(window);
}

void VulkanWindow::destroyPlatformSpecificWindow()
{
    DestroyWindow(window);
    UnregisterClass(className.c_str(), instance);
}

void VulkanWindow::updatePlatformSpecificWindow()
{
    MSG msg;
    if(PeekMessage(&msg, window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

#endif // VK_USE_PLATFORM_WIN32_KHR
