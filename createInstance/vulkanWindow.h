#pragma once

#include "platform.h"
#include <string>
#include <vector>

class Renderer;

class VulkanWindow
{

public:
    VulkanWindow(Renderer *renderer, uint32_t sizeX, uint32_t sizeY, std::string name);
    ~VulkanWindow();

    void close();
    bool update();

private:
    bool isRunning = true;
    uint32_t surfaceSizeX = 512;
    uint32_t surfaceSizeY = 512;
    std::string windowName;
    Renderer *renderer = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    VkSurfaceFormatKHR surfaceFormat = {};

    #if VK_USE_PLATFORM_WIN32_KHR

    HINSTANCE hInstance = NULL;
    HWND hWindow = NULL;
    std::string className;
    static uint64_t win32ClassIdCounter;

    #endif // VK_USE_PLATFORM_WIN32_KHR

    void initPlatformSpecificWindow();
    void destroyPlatformSpecificWindow();
    void updatePlatformSpecificWindow();
    void initPlatformSpecificSurface();
    void destroyPlatformSpecificSurface();
    void initSurface();
    void destroySurface();
    void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
};
