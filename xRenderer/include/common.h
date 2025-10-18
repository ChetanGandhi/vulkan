#pragma once

#include "platform.h"
#include <glm/glm.hpp>

// clang-format off

#define XR_FREE(pointer) { if (pointer) { free(pointer); pointer = VK_NULL_HANDLE; } }

// clang-format on

typedef struct XrMaterial
{
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    float shininess;
} XrMaterial;

typedef struct XrBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize memorySize = 0;
} XrBuffer;

typedef struct XrImage
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceSize memorySize = 0;
} XrImage;

typedef struct XrTexture
{
    VkExtent2D extent;
    uint32_t channels;
    uint32_t mipLevels;
    VkFormat format;

    XrImage *image = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
} XrTexture;

typedef struct XrSwapchainSupport
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;

    VkSurfaceFormatKHR *surfaceFormats = VK_NULL_HANDLE;
    uint32_t surfaceFormatsCount = 0;

    VkPresentModeKHR *presentModes = VK_NULL_HANDLE;
    uint32_t presentModesCount = 0;
} XrSwapchainSupport;

typedef struct XrPhysicalDevice
{
    uint32_t rank = 0;
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;
    bool hasSeparatePresentQueue = false;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
} XrPhysicalDevice;

typedef struct XrUniformBuffer
{
    // Matrices
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    // Lighting
    glm::vec4 lightAmbient;
    glm::vec4 lightDiffuse;
    glm::vec4 lightSpecular;
    glm::vec4 lightPosition;

    // Material
    glm::vec4 materialAmbient;
    glm::vec4 materialDiffuse;
    glm::vec4 materialSpecular;
    float materialShininess;
} XrUniformBuffer;
