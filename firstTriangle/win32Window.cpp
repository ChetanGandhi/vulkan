#pragma once

#include "buildParam.h"
#include "platform.h"
#include "vulkanWindow.h"
#include "utils.h"

#include <assert.h>
#include <iostream>
#include <chrono>

#if VK_USE_PLATFORM_WIN32_KHR

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch(iMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
        break;

        case WM_ACTIVATE:
            isActive = (HIWORD(wParam) == 0);
        break;

        case WM_SIZE:
            resize(LOWORD(lParam), HIWORD(lParam));
        break;

        case WM_KEYDOWN:
            switch(wParam)
            {
                case VK_ESCAPE:
                    isEscapeKeyPressed = true;;
                break;

                // 0x46 is hex value for key 'F' or 'f'
                case 0x46:
                    isFullscreen = !isFullscreen;
                    toggleFullscreen(isFullscreen);
                break;

                default:
                break;
            }

        break;

        default:
        break;
    }

    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInsatnce, LPSTR lpszCmdLine, int nCmdShow)
{
    Logger::init("debug.log");

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | First Triangle";

    surfaceSize.width = 800;
    surfaceSize.height = 600;

    hGlobalInstance = hInstance;

    initPlatformSpecificWindow();
    initilizeVulkan();

    int returnCode = mainLoop();

    cleanUp();
    Logger::close();

    return returnCode;
}

void initPlatformSpecificWindow()
{
    WNDCLASSEX wndclassex {};

    assert(surfaceSize.width > 0);
    assert(surfaceSize.height > 0);

    className = windowName + "_" + std::to_string(win32ClassIdCounter);
    win32ClassIdCounter++;

    wndclassex.cbSize = sizeof(WNDCLASSEX);
    wndclassex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclassex.cbClsExtra = 0;
    wndclassex.cbWndExtra = 0;
    wndclassex.lpfnWndProc = WndProc;
    wndclassex.hInstance = hGlobalInstance;
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

    DWORD dwStyleExtra = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;

    RECT windowRect = {0, 0, LONG(surfaceSize.width), LONG(surfaceSize.height)};
    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwStyleExtra);

    hWindow = CreateWindowEx(dwStyleExtra,
        className.c_str(),
        windowTitle.c_str(),
        dwStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        hGlobalInstance,
        NULL);

    if(!hWindow)
    {
        assert(0 && "Cannot create window.\n");
        fflush(stdout);
        std::exit(EXIT_FAILURE);
    }

    ShowWindow(hWindow, SW_SHOW);
    SetForegroundWindow(hWindow);
    SetFocus(hWindow);
}

void destroyPlatformSpecificWindow()
{
    DestroyWindow(hWindow);
    UnregisterClass(className.c_str(), hGlobalInstance);
}

void initilizeVulkan()
{
    renderer = new Renderer(surfaceSize);

    initPlatformSpecificSurface();

    renderer->setSurface(surface);
    renderer->initDevice();
    renderer->initLogicalDevice();
    renderer->initSwapchain();
    renderer->initSwapchainImageViews();
    // renderer->initDepthStencilImage();
    renderer->initRenderPass();
    renderer->initGraphicsPipline();
    renderer->initFrameBuffers();
    renderer->initCommandPool();
    renderer->initVertexBuffer();
    renderer->initCommandBuffers();
    renderer->initSynchronizations();
}

void cleanUp()
{
    if(isFullscreen)
    {
        dwStyle = GetWindowLong(hWindow, GWL_STYLE);
        SetWindowLong(hWindow, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWindow, &wpPrev);
        SetWindowPos(hWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowCursor(TRUE);
    }

    renderer->waitForIdle();
    renderer->destroySynchronizations();
    renderer->destroyCommandBuffers();
    renderer->destroyVertexBuffer();
    renderer->destroyCommandPool();
    renderer->destroyFrameBuffers();
    renderer->destroyGraphicsPipline();
    renderer->destroyRenderPass();
    // renderer->destoryDepthStencilImage();
    renderer->destroySwapchainImageViews();
    renderer->destroySwapchain();

    destroyPlatformSpecificSurface();

    renderer->destroyDevice();
    delete renderer;

    destroyPlatformSpecificWindow();
}

int mainLoop()
{
    MSG msg;
    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;
    TCHAR fpsTitle[50];

    while(isRunning)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                isRunning = false;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            if(isActive)
            {
                if(isEscapeKeyPressed)
                {
                    isRunning = false;
                }
                else
                {
                    ++frameCounter;

                    if(lastTime + std::chrono::seconds(1) < timer.now())
                    {
                        lastTime = timer.now();
                        fps = frameCounter;
                        frameCounter = 0;
                        wsprintf(fpsTitle, "Vulkan Window | First Triangle | FPS - %d", fps);
                        SetWindowText(hWindow, fpsTitle);
                    }

                    renderer->render();
                }
            }
        }
    }

    return (int)msg.wParam;
}

void initPlatformSpecificSurface()
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = nullptr;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.hinstance = hGlobalInstance;
    surfaceCreateInfo.hwnd = hWindow;

    VkResult result = vkCreateWin32SurfaceKHR(renderer->getVulkanInstance(), &surfaceCreateInfo, nullptr, &surface);

    checkError(result, __FILE__, __LINE__);
}

void destroyPlatformSpecificSurface()
{
    vkDestroySurfaceKHR(renderer->getVulkanInstance(), surface, nullptr);
}

void resize(uint32_t width, uint32_t height)
{
    if(width == 0 || height == 0)
    {
        return;
    }

    surfaceSize.width = width;
    surfaceSize.height = height;

    if(renderer != nullptr)
    {
        renderer->setSurfaceSize(surfaceSize);
        renderer->recreateSwapChain();
    }
}

void toggleFullscreen(bool isFullscreen)
{
    MONITORINFO monitorInfo;
    dwStyle = GetWindowLong(hWindow, GWL_STYLE);

    if(isFullscreen)
    {
        if(dwStyle & WS_OVERLAPPEDWINDOW)
        {
            monitorInfo = { sizeof(MONITORINFO) };

            if(GetWindowPlacement(hWindow, &wpPrev) && GetMonitorInfo(MonitorFromWindow(hWindow, MONITORINFOF_PRIMARY), &monitorInfo))
            {
                SetWindowLong(hWindow, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(hWindow, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }

        ShowCursor(FALSE);
    }
    else
    {
        SetWindowLong(hWindow, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWindow, &wpPrev);
        SetWindowPos(hWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowCursor(TRUE);
    }
}

void onEscapeKeyPressed()
{
    isRunning = false;
}

#endif // VK_USE_PLATFORM_WIN32_KHR
