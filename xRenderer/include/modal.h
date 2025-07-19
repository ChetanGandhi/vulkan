#pragma once

#include "platform.h"
#include "common.h"
#include "debugger.h"
#include "vertex.h"

namespace xr
{
    struct Modal {
      public:
        std::vector<VkDescriptorSet> descriptorSets;
        xr::UniformBufferObject ubo;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> vertexIndices;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
    };
} // namespace xr
