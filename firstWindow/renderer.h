#pragma once

#include "platform.h"
#include <vector>
#include <string>

class VulkanWindow;

class Renderer
{

public:
    Renderer();
    ~Renderer();

    VulkanWindow* createVulkanVindow(uint32_t sizeX, uint32_t sizeY, std::string name, std::string title);

    bool run();

    const VkInstance getVulkanInstance() const;
    const VkPhysicalDevice getVulkanPhysicalDevice() const;
    const VkDevice getVulkanDevice() const;
    const VkQueue getVulkanQueue() const;
    const VkPhysicalDeviceProperties &getVulkanPhysicalDeviceProperties() const;
    const VkPhysicalDeviceMemoryProperties &getVulkanPhysicalDeviceMemoryProperties() const;

    const uint32_t getGraphicsFamilyIndex() const;

 private:
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    struct GpuDetails {
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
    } gpuDetails;

    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    VulkanWindow *vulkanWindow = nullptr;

    struct QueueFamilyIndices {
        uint32_t graphicsFamilyIndex = UINT32_MAX;
        uint32_t presentFamilyIndex = UINT32_MAX;
        bool hasSeparatePresentQueue = false;
    } queueFamilyIndices;

    std::vector<const char*> instanceLayerList;
    std::vector<const char*> deviceLayerList;
    std::vector<const char*> instanceExtensionList;
    std::vector<const char*> deviceExtensionList;

    void setupDebugLayer();
    void setupLayersAndExtensions();

    void enableDebug();
    void disableDebug();

    void initInstance();
    void destroyInstance();

    void listAllPhysicalDevices(std::vector<GpuDetails> *gpuDetailsList);
    bool isDeviceSuitable(VkPhysicalDevice device);

    void initDevice();
    void initLogicalDevice();
    void destroyDevice();

    // Debug methods

    void printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount);
    void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
    void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
};
