#pragma once

#include "platform.h"
#include <string>
#include <vector>

class Renderer;

class VulkanWindow
{

public:
    VulkanWindow(Renderer *renderer, uint32_t width, uint32_t height, std::string name, std::string title);
    ~VulkanWindow();

    void close();
    bool update();

    void beginRendering();
    void endRendering(std::vector<VkSemaphore> waitSemaphores);

    VkRenderPass getVulkanRenderPass();
    VkFramebuffer getVulkanActiveFramebuffer();
    VkExtent2D getVulkanSurfaceSize();

private:
    bool isRunning = true;
    bool stencilAvailable = false;

    struct SurfaceSize {
        uint32_t width = 512;
        uint32_t height = 512;
    } surfaceSize;

    uint32_t swapchainImageCount = 2;
    uint32_t activeSwapchainImageId = UINT32_MAX;

    std::string windowName;
    std::string windowTitle;

    Renderer *renderer = nullptr;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFence swapchainImageAvailable = VK_NULL_HANDLE;

    struct DepthStencil {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    } depthStencil;

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    VkSurfaceFormatKHR surfaceFormat = {};

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

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

    void initFrameBuffers();
    void destroyFrameBuffers();

    void initSynchronizations();
    void destroySynchronizations();

    // Debug methods

    void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
    void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
};
