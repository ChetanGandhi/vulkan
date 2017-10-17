// #pragma once

#include "platform.h"

#if defined(VK_USE_PLATFORM_XCB_KHR)

#include "vulkanWindow.h"

int main(void)
{
    Logger::init("debug_linux.log");

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | XWindows";

    surfaceSize.width = 800;
    surfaceSize.height = 600;

    initializePlatformSpecificWindow();

    int returnCode = mainLoop();

    cleanUp();
    Logger::close();

    return returnCode;
}

void initializePlatformSpecificWindow()
{
    assert(surfaceSize.width > 0);
    assert(surfaceSize.height > 0);
}

void destroyPlatformSpecificWindow()
{

}

void initializeVulkan()
{

}

void cleanUp()
{
    LOG("---------- Cleanup Started ----------");

    if(isFullscreen)
    {
        isFullscreen = false;
        toggleFullscreen(isFullscreen);
    }

    destroyPlatformSpecificWindow();
}

int mainLoop()
{
    bool done = false;

    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;

    char *fpsTitle = (char *)malloc(sizeof(windowTitle.c_str()) + sizeof(fps) + 100);

    while(!done)
    {
        done = (isCloseButtonClicked || isEscapeKeyPressed);

        ++frameCounter;

        if(lastTime + std::chrono::seconds(1) < timer.now())
        {
            lastTime = timer.now();
            fps = frameCounter;
            frameCounter = 0;
            sprintf(fpsTitle, "%s | FPS - %ld", windowTitle.c_str(), fps);
            printf("%s\n", fpsTitle);
        }
    }

    free(fpsTitle);
    fpsTitle = nullptr;

    return EXIT_SUCCESS;
}

void initPlatformSpecificSurface()
{

}

void destroyPlatformSpecificSurface()
{

}

void resize(uint32_t width, uint32_t height)
{
    if(width == 0 || height == 0)
    {
        return;
    }

    surfaceSize.width = width;
    surfaceSize.height = height;
}

void toggleFullscreen(bool isFullscreen)
{

}

void onEscapeKeyPressed()
{
    isRunning = false;
}

#endif // VK_USE_PLATFORM_XCB_KHR
