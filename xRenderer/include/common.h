#pragma once

#include "platform.h"
#include <glm/glm.hpp>

typedef struct XrSwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
} XrSwapchainSupportDetails;

typedef struct XrPhysicalDevice
{
    uint32_t rank = 0;
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;
    bool hasSeparatePresentQueue = false;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties = {};
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
} XrPhysicalDevice;

typedef struct XrUniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
} XrUniformBufferObject;
