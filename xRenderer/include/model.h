#pragma once

#include "platform.h"
#include "common.h"
#include "vertex.h"
#include "texture.h"

typedef struct XrModel
{
    std::vector<XrVertex *> vertices = {};
    std::vector<uint32_t> vertexIndices = {};

    std::vector<VkDescriptorSet> descriptorSets = {};

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    XrUniformBufferObject ubo = {};
    std::vector<VkBuffer> uniformBuffers = {};
    std::vector<VkDeviceMemory> uniformBuffersMemory = {};

    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
} XrModel;
