#pragma once

#include "platform.h"
#include <glm/glm.hpp>

namespace xr
{
    struct SurfaceSize
    {
        uint32_t width = 512;
        uint32_t height = 512;
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct GpuDetails
    {
        uint32_t rank = 0;
        uint32_t graphicsFamilyIndex = UINT32_MAX;
        uint32_t presentFamilyIndex = UINT32_MAX;
        bool hasSeparatePresentQueue = false;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
    };

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };
} // namespace xr
