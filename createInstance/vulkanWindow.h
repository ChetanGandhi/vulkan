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
    bool stencilAvailable = false;

    uint32_t surfaceSizeX = 512;
    uint32_t surfaceSizeY = 512;
    uint32_t swapchainImageCount = 2;

    std::string windowName;

    Renderer *renderer = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkImage depthStencilImage = VK_NULL_HANDLE;
    VkImageView depthStencilImageView = VK_NULL_HANDLE;
    VkDeviceMemory depthStencilImageMemory = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    VkSurfaceFormatKHR surfaceFormat = {};

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;

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

    void initSwapchain();
    void destroySwapchain();

    void initSwapchainImages();
    void destroySwapchainImages();

    void initDepthStencilImage();
    void destoryDepthStencilImage();

    void initRenderPass();
    void destroyRenderPass();

    // Debug methods

    void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
    void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
};
