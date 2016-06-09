#pragma once

#include "platform.h"
#include <vector>

class VulkanWindow;

class Renderer
{

public:
    Renderer();
    ~Renderer();

    VulkanWindow* createVulkanVindow(uint32_t sizeX, uint32_t sizeY, std::string name);
    bool run();

// private:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties gpuProperties {};
    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    VulkanWindow *vulkanWindow = nullptr;

    uint32_t graphicsFamilyIndex = 0;

    std::vector<const char*> v;
    std::vector<const char*> instanceLayerList;
    std::vector<const char*> deviceLayerList;
    std::vector<const char*> instanceExtensionList;
    std::vector<const char*> deviceExtensionList;

    void out();
    void setupDebugLayer();
    void enableDebud();
    void disableDebug();
    void initInstance();
    void destroyInstance();
    void initDevice();
    void destroyDevice();
    void printGpuProperties(VkPhysicalDeviceProperties *properties);
    void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
    void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
};
