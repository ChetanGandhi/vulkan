#pragma once

#include "platform.h"
#include "Renderer.h"
#include "common.h"
#include <string>
#include <vector>

class VulkanWindow
{

public:
    VulkanWindow(uint32_t width, uint32_t height, std::string name, std::string title);
    ~VulkanWindow();

    bool run();
    bool update();

    void close();
    void beginRendering();
    void endRendering(std::vector<VkSemaphore> waitSemaphores);

    VkExtent2D getVulkanSurfaceSize();

private:
    bool isRunning = true;

    SurfaceSize surfaceSize;

    std::string windowName;
    std::string windowTitle;

    Renderer *renderer = nullptr;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    #if VK_USE_PLATFORM_WIN32_KHR

    HINSTANCE hInstance = NULL;
    HWND hWindow = NULL;

    std::string className;

    static uint64_t win32ClassIdCounter;

    #endif // VK_USE_PLATFORM_WIN32_KHR

    void createVulkanVindow(uint32_t sizeX, uint32_t sizeY, std::string name, std::string title);

    void initPlatformSpecificWindow();
    void destroyPlatformSpecificWindow();

    void updatePlatformSpecificWindow();

    void initPlatformSpecificSurface();
    void destroyPlatformSpecificSurface();
};
