#define GLM_FORCE_CXX17

#include <xRenderer/platform.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)

#include <xRenderer/vulkanWindow.h>
#include "resource.h"

xr::UniformBufferObject ubo;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
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
            switch (wParam)
            {
                case VK_ESCAPE:
                    isEscapeKeyPressed = true;
                    ;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    xr::Logger::initialize("debug_win32.log");

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | Win32";

    vkState = new xr::VulkanState();
    vkState->surfaceSize = {};
    vkState->surfaceSize.width = 800;
    vkState->surfaceSize.height = 600;
    vkState->vertexShaderFilePath = "../shaders/vert.spv";
    vkState->fragmentShaderFile = "../shaders/frag.spv";

    hGlobalInstance = hInstance;

    initializePlatformSpecificWindow();
    initializeVulkan();

    int returnCode = mainLoop();

    cleanUp();
    xr::Logger::close();

    return returnCode;
}

void initializePlatformSpecificWindow()
{
    WNDCLASSEX wndclassex{};

    className = windowName + std::string("_") + std::to_string(win32ClassIdCounter);
    win32ClassIdCounter++;

    wndclassex.cbSize = sizeof(WNDCLASSEX);
    wndclassex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclassex.cbClsExtra = 0;
    wndclassex.cbWndExtra = 0;
    wndclassex.lpfnWndProc = WndProc;
    wndclassex.hInstance = hGlobalInstance;
    wndclassex.hIcon = LoadIcon(hGlobalInstance, MAKEINTRESOURCE(CP_ICON));
    wndclassex.hIconSm = LoadIcon(hGlobalInstance, MAKEINTRESOURCE(CP_ICON_SMALL));
    wndclassex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclassex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclassex.lpszClassName = className.c_str();
    wndclassex.lpszMenuName = NULL;

    if (!RegisterClassEx(&wndclassex))
    {
        assert(1 && "Cannot register window class.\n");
        logf("Error: Unable to open XgDisplay.\n");
        // TODO: Call cleanup.
        fflush(stdout);
        std::exit(EXIT_FAILURE);
    }

    DWORD dwStyleExtra = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;

    RECT windowRect = { 0, 0, LONG(vkState->surfaceSize.width), LONG(vkState->surfaceSize.height) };
    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwStyleExtra);

    hWindow = CreateWindowEx(
        dwStyleExtra,
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
        NULL
    );

    if (!hWindow)
    {
        assert(0 && "Cannot create window.\n");
        logf("Cannot create window.\n");
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

void initializeVulkan()
{
    renderer = new xr::Renderer(vkState);

    initPlatformSpecificSurface(&(vkState->instance->vkInstance), &(vkState->surface));

    renderer->initDevice();
    renderer->initLogicalDevice();
    renderer->initSwapchain();
    renderer->initSwapchainImageViews();
    renderer->initRenderPass();
    renderer->initDescriptorSetLayout();
    renderer->initGraphicsPiplineCache();
    renderer->initGraphicsPipline();
    renderer->initCommandPool();
    renderer->initDepthStencilImage();
    renderer->initMSAAColorImage();
    renderer->initFrameBuffers();
    renderer->initTextureImage("../resources/textures/chalet/chalet.jpg");
    renderer->initTextureImageView();
    renderer->initTextureSampler();
    renderer->loadModel("../resources/models/chalet/chalet.obj");
    renderer->initVertexBuffer();
    renderer->initIndexBuffer();
    renderer->initUniformBuffers();
    renderer->initDescriptorPool();
    renderer->initDescriptorSets();
    renderer->initCommandBuffers();
    renderer->initSynchronizations();
}

void cleanUp()
{
    logf("---------- Cleanup started ----------");

    if (isFullscreen)
    {
        isFullscreen = false;
        toggleFullscreen(isFullscreen);
    }

    if (renderer != nullptr)
    {
        renderer->waitForIdle();
        renderer->destroySynchronizations();
        renderer->destroyCommandBuffers();
        renderer->destroyDescriptorSets();
        renderer->destroyDescriptorPool();
        renderer->destroyUniformBuffers();
        renderer->destroyIndexBuffer();
        renderer->destroyVertexBuffer();
        renderer->destoryTextureSampler();
        renderer->destroyTextureImageView();
        renderer->destroyTextureImage();
        renderer->destroyFrameBuffers();
        renderer->destoryMSAAColorImage();
        renderer->destoryDepthStencilImage();
        renderer->destroyCommandPool();
        renderer->destroyGraphicsPipline();
        renderer->destroyGraphicsPiplineCache();
        renderer->destroyDescriptorSetLayout();
        renderer->destroyRenderPass();
        renderer->destroySwapchainImageViews();
        renderer->destroySwapchain();
        renderer->destroyDevice();
    }

    // The surface need to be destoyed before instance is deleted.
    destroyPlatformSpecificSurface();

    // Instance is deleted in destructor of Renderer class.
    delete renderer;
    renderer = nullptr;

    delete vkState;
    vkState = nullptr;

    destroyPlatformSpecificWindow();

    logf("---------- Cleanup done ----------");
}

void update()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, -6.0f, 0.0f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // To push object deep into screen, modify the eye matrix to have more positive (greater) value at z-axis.
    memset((void *)&ubo, 0, sizeof(xr::UniformBufferObject));
    ubo.model = translationMatrix * rotationMatrix;
    ubo.view = glm::lookAt(glm::vec3(6.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(glm::radians(45.0f), (float)vkState->surfaceSize.width / (float)vkState->surfaceSize.height, 0.1f, 100.0f);

    // The GLM is designed for OpenGL, where the Y coordinate of the clip coordinate is inverted.
    // If we do not fix this then the image will be rendered upside-down.
    // The easy way to fix this is to flip the sign on the scaling factor of Y axis
    // in the projection matrix.
    ubo.projection[1][1] *= -1.0f;
}

int mainLoop()
{
    MSG msg;
    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;
    std::string fpsTitle;

    while (isRunning)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
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
            if (isActive)
            {
                if (isEscapeKeyPressed)
                {
                    isRunning = false;
                }
                else
                {
                    ++frameCounter;

                    if (lastTime + std::chrono::seconds(1) < timer.now())
                    {
                        lastTime = timer.now();
                        fps = frameCounter;
                        frameCounter = 0;
                        fpsTitle = windowTitle + std::string(" | FPS - ") + std::to_string(fps);
                        SetWindowText(hWindow, fpsTitle.c_str());
                    }

                    // This will also update uniform buffer as per current inflight image.
                    update();
                    renderer->render(&ubo);
                }
            }
        }
    }

    return (int)msg.wParam;
}

void initPlatformSpecificSurface(VkInstance *instance, VkSurfaceKHR *surface)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = nullptr;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.hinstance = hGlobalInstance;
    surfaceCreateInfo.hwnd = hWindow;

    VkResult result = vkCreateWin32SurfaceKHR(*instance, &surfaceCreateInfo, nullptr, surface);
    CHECK_ERROR(result);
}

void destroyPlatformSpecificSurface()
{
    vkDestroySurfaceKHR(vkState->instance->vkInstance, vkState->surface, nullptr);
    vkState->surface = VK_NULL_HANDLE;
}

void resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    if (width == vkState->surfaceSize.width && height == vkState->surfaceSize.height)
    {
        return;
    }

    vkState->surfaceSize.width = width;
    vkState->surfaceSize.height = height;

    if (renderer != nullptr)
    {
        renderer->recreateSwapChain();
    }
}

void toggleFullscreen(bool isFullscreen)
{
    MONITORINFO monitorInfo;
    dwStyle = GetWindowLong(hWindow, GWL_STYLE);

    if (isFullscreen)
    {
        if (dwStyle & WS_OVERLAPPEDWINDOW)
        {
            monitorInfo = { sizeof(MONITORINFO) };

            if (GetWindowPlacement(hWindow, &wpPrev) && GetMonitorInfo(MonitorFromWindow(hWindow, MONITORINFOF_PRIMARY), &monitorInfo))
            {
                SetWindowLong(hWindow, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(
                    hWindow,
                    HWND_TOP,
                    monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.top,
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                    SWP_NOZORDER | SWP_FRAMECHANGED
                );
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

#endif // VK_USE_PLATFORM_WIN32_KHR
