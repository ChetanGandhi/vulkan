#pragma once

#include "platform.h"
#include <string>

class VulkanWindow
{

public:
    VulkanWindow(uint32_t sizeX, uint32_t sizeY, std::string name);
    ~VulkanWindow();

    void close();
    bool update();

private:
    bool isRunning = true;
    uint32_t surfaceSizeX = 512;
    uint32_t surfaceSizeY = 512;
    std::string windowName;

    #if VK_USE_PLATFORM_WIN32_KHR

    HINSTANCE instance = NULL;
    HWND window = NULL;
    std::string className;
    static uint64_t win32ClassIdCounter;

    #endif // VK_USE_PLATFORM_WIN32_KHR

    void initPlatformSpecificWindow();
    void destroyPlatformSpecificWindow();
    void updatePlatformSpecificWindow();
    void initPlatformSpecificSurface();
};
