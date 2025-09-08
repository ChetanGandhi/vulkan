#include <xRenderer/platform.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)

#include <xRenderer/logger.h>
#include <xRenderer/model.h>
#include <xRenderer/texture.h>

#include "lib/stb/stb_image.h"

#include "vulkanWindow.h"
#include "resource.h"
#include "utils.h"

static uint64_t win32ClassIdCounter = 0;
std::string className;
bool isActive = false;

HINSTANCE hGlobalInstance = NULL;
HWND hWindow = NULL;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev = {sizeof(WINDOWPLACEMENT)};

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow);

XRModel *homeModel = nullptr;
XRTexture *homeTexture = nullptr;

XRModel *vikingRoomModel = nullptr;
XRTexture *vikingRoomTexture = nullptr;

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
    XRLogger::initialize("debug_win32.log");

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | Win32";

    context = new XrContext();
    context->surfaceSize = {};
    context->surfaceSize.width = 800;
    context->surfaceSize.height = 600;
    context->vertexShaderFilePath = "../shaders/vert.spv";
    context->fragmentShaderFile = "../shaders/frag.spv";

    hGlobalInstance = hInstance;

    initializePlatformSpecificWindow();
    initializeVulkan();

    int returnCode = mainLoop();

    cleanUp();
    XRLogger::close();

    return returnCode;
}

void initializePlatformSpecificWindow()
{
    WNDCLASSEX wndclassex = {};

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
        LOG_INFO("Error: Unable to open XgDisplay.\n");
        // TODO: Call cleanup.
        fflush(stdout);
        std::exit(EXIT_FAILURE);
    }

    DWORD dwStyleExtra = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;

    RECT windowRect = {0, 0, LONG(context->surfaceSize.width), LONG(context->surfaceSize.height)};
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
        LOG_INFO("Cannot create window.\n");
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
    context->debugger = new XRDebugger();

    VkApplicationInfo applicationInfo;
    memset((void *), &applicationInfo, 0, sizeof(VkApplicationInfo));
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = nullptr;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = "Vulkan";
    applicationInfo.pEngineName = nullptr;
    applicationInfo.engineVersion = 0;

    xrInitInstance(context, &applicationInfo);

    initPlatformSpecificSurface(&(context->instance->vkInstance), &(context->surface));

    xrInitDevice(context);
    xrInitLogicalDevice(context);
    xrInitSwapchain(context);
    xrInitSwapchainImageViews(context);
    xrInitRenderPass(context);
    xrInitDescriptorSetLayout(context);
    xrInitGraphicsPiplineCache(context);
    xrInitGraphicsPipline(context);
    xrInitCommandPool(context);
    xrInitDepthStencilImage(context);
    xrInitMSAAColorImage(context);
    xrInitFrameBuffers(context);

    homeModel = new XRModel();
    bool homeModelLoaded = loadModal("../resources/models/chalet/chalet.obj", homeModel);

    if (!homeModelLoaded)
    {
        assert(0 && "Not able to load home model.");
    }

    LOG_INFO("Home model loaded");

    homeTexture = (XRTexture *)malloc(sizeof(XRTexture));
    memset((void *)homeTexture, 0, sizeof(XRTexture));

    stbi_uc *homeTextureData = nullptr;
    loadTexture("../resources/textures/chalet/chalet.jpg", homeTexture, &homeTextureData);

    if (!homeTextureData)
    {
        assert(0 && "Not able to load home texture.");
    }

    xrInitTextureImage(context, homeModel, homeTexture, homeTextureData);
    LOG_INFO("Home texture loaded");

    // Free the texture data as no longer required
    if (homeTextureData)
    {
        free(homeTextureData);
        homeTextureData = nullptr;
        LOG_INFO("Free home texture data");
    }

    xrInitTextureImageView(context, homeModel, homeTexture);
    xrInitTextureSampler(context, homeModel, homeTexture);
    xrInitVertexBuffer(context, homeModel);
    xrInitIndexBuffer(context, homeModel);
    xrInitUniformBuffers(context, homeModel);

    vikingRoomModel = new XRModel();
    bool vikingRoomModelLoaded = loadModal("../resources/models/vikingRoom/vikingRoom.obj", vikingRoomModel);

    if (!vikingRoomModelLoaded)
    {
        assert(0 && "Not able to load viking room model.");
    }

    LOG_INFO("Viking room model loaded");

    vikingRoomTexture = (XRTexture *)malloc(sizeof(XRTexture));
    memset((void *)vikingRoomTexture, 0, sizeof(XRTexture));

    stbi_uc *vikingRoomTextureData = nullptr;
    loadTexture("../resources/textures/vikingRoom/vikingRoom.png", vikingRoomTexture, &vikingRoomTextureData);

    if (!vikingRoomTextureData)
    {
        assert(0 && "Not able to load viking room texture.");
    }

    xrInitTextureImage(context, vikingRoomModel, vikingRoomTexture, vikingRoomTextureData);
    LOG_INFO("Viking room texture loaded");

    // Free the texture data as no longer required
    if (vikingRoomTextureData)
    {
        free(vikingRoomTextureData);
        vikingRoomTextureData = nullptr;
        LOG_INFO("Free viking room texture loaded");
    }

    xrInitTextureImageView(context, vikingRoomModel, vikingRoomTexture);
    xrInitTextureSampler(context, vikingRoomModel, vikingRoomTexture);
    xrInitVertexBuffer(context, vikingRoomModel);
    xrInitIndexBuffer(context, vikingRoomModel);
    xrInitUniformBuffers(context, vikingRoomModel);

    xrInitDescriptorPool(context, 2);
    xrInitDescriptorSets(context, {homeModel, vikingRoomModel});
    xrInitCommandBuffers(context, {homeModel, vikingRoomModel});
    xrInitSynchronizations(context);
}

void cleanUp()
{
    LOG_INFO("---------- Cleanup started ----------");

    if (isFullscreen)
    {
        isFullscreen = false;
        toggleFullscreen(isFullscreen);
    }

    if (renderer != nullptr)
    {
        xrWaitForIdle(context);
        xrDestroySynchronizations(context);
        xrDestroyCommandBuffers(context);
        xrDestroyDescriptorSets(context, {homeModel, vikingRoomModel});
        xrDestroyDescriptorPool(context);

        xrDestroyUniformBuffers(context, homeModel);
        xrDestroyIndexBuffer(context, homeModel);
        xrDestroyVertexBuffer(context, homeModel);
        xrDestroyTextureSampler(context, homeModel);
        xrDestroyTextureImageView(context, homeModel);
        xrDestroyTextureImage(context, homeModel);

        xrDestroyUniformBuffers(context, vikingRoomModel);
        xrDestroyIndexBuffer(context, vikingRoomModel);
        xrDestroyVertexBuffer(context, vikingRoomModel);
        xrDestroyTextureSampler(context, vikingRoomModel);
        xrDestroyTextureImageView(context, vikingRoomModel);
        xrDestroyTextureImage(context, vikingRoomModel);

        xrDestroyFrameBuffers(context);
        xrDestroyMSAAColorImage(context);
        xrDestroyDepthStencilImage(context);
        xrDestroyCommandPool(context);
        xrDestroyGraphicsPipline(context);
        xrDestroyGraphicsPiplineCache(context);
        xrDestroyDescriptorSetLayout(context);
        xrDestroyRenderPass(context);
        xrDestroySwapchainImageViews(context);
        xrDestroySwapchain(context);
        xrDestroyDevice(context);
    }

    // The surface need to be destroyed before instance is deleted.
    destroyPlatformSpecificSurface();

    if (homeTexture)
    {
        free(homeTexture);
        homeTexture = nullptr;
    }

    if (vikingRoomTexture)
    {
        free(vikingRoomTexture);
        vikingRoomTexture = nullptr;
    }

    if (homeModel)
    {
        delete homeModel;
        homeModel = nullptr;
    }

    if (vikingRoomModel)
    {
        delete vikingRoomModel;
        vikingRoomModel = nullptr;
    }

    if (renderer)
    {
        xrDestroyInstance(context);

        delete renderer;
        renderer = nullptr;
    }

    if (context->debugger)
    {
        delete context->debugger;
        context->debugger = nullptr;
    }

    if (context)
    {
        delete context;
        context = nullptr;
    }

    destroyPlatformSpecificWindow();

    LOG_INFO("---------- Cleanup done ----------");
}

void updateHomeModel()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, -1.0f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // To push object deep into screen, modify the eye matrix to have more positive (greater) value at z-axis.
    memset((void *)&(homeModel->ubo), 0, sizeof(XRUniformBufferObject));
    homeModel->ubo.model = translationMatrix * rotationMatrix;
    homeModel->ubo.view = glm::lookAt(glm::vec3(6.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    homeModel->ubo.projection = glm::perspective(glm::radians(45.0f), (float)context->surfaceSize.width / (float)context->surfaceSize.height, 0.1f, 100.0f);

    // The GLM is designed for OpenGL, where the Y coordinate of the clip coordinate is inverted.
    // If we do not fix this then the image will be rendered upside-down.
    // The easy way to fix this is to flip the sign on the scaling factor of Y axis
    // in the projection matrix.
    homeModel->ubo.projection[1][1] *= -1.0f;
}

void updateVikingRoomModel()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 1.5f, -1.0f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // To push object deep into screen, modify the eye matrix to have more positive (greater) value at z-axis.
    memset((void *)&(vikingRoomModel->ubo), 0, sizeof(XRUniformBufferObject));
    vikingRoomModel->ubo.model = translationMatrix * rotationMatrix;
    vikingRoomModel->ubo.view = glm::lookAt(glm::vec3(6.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    vikingRoomModel->ubo.projection =
        glm::perspective(glm::radians(45.0f), (float)context->surfaceSize.width / (float)context->surfaceSize.height, 0.1f, 100.0f);

    // The GLM is designed for OpenGL, where the Y coordinate of the clip coordinate is inverted.
    // If we do not fix this then the image will be rendered upside-down.
    // The easy way to fix this is to flip the sign on the scaling factor of Y axis
    // in the projection matrix.
    vikingRoomModel->ubo.projection[1][1] *= -1.0f;
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

                    updateHomeModel();
                    updateVikingRoomModel();
                    xrRender(context, {homeModel, vikingRoomModel});
                }
            }
        }
    }

    return (int)msg.wParam;
}

void initPlatformSpecificSurface(VkInstance *instance, VkSurfaceKHR *surface)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
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
    vkDestroySurfaceKHR(context->instance->vkInstance, context->surface, nullptr);
    context->surface = VK_NULL_HANDLE;
}

void resize(uint32_t width, uint32_t height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (width == context->surfaceSize.width && height == context->surfaceSize.height)
    {
        return;
    }

    context->surfaceSize.width = width;
    context->surfaceSize.height = height;

    if (renderer != nullptr)
    {
        xrRecreateSwapChain(context, {homeModel, vikingRoomModel});
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
            monitorInfo = {sizeof(MONITORINFO)};

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
