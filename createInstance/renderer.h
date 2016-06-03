#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Renderer
{

public:
    Renderer();
    ~Renderer();

private:
    VkInstance instance = nullptr;
    VkPhysicalDevice gpu = nullptr;
    VkDevice device = nullptr;
    VkPhysicalDeviceProperties gpuProperties {};
    VkDebugReportCallbackEXT debugReport = nullptr;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    uint32_t graphicsFamilyIndex = 0;

    std::vector<const char*> v;
    std::vector<const char*> instanceLayerList;
    std::vector<const char*> deviceLayerList;
    std::vector<const char*> instanceExtensionList;
    std::vector<const char*> deviceExtensionList;

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
