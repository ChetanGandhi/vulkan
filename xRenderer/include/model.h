#pragma once

#include "platform.h"
#include "common.h"
#include "vertex.h"

typedef struct XrModel
{
    XrVertex *vertices = VK_NULL_HANDLE;
    uint32_t verticesCount = 0;

    uint32_t *vertexIndices = VK_NULL_HANDLE;
    uint32_t vertexIndicesCount = 0;

    VkDescriptorSet *descriptorSets = VK_NULL_HANDLE;
    uint32_t descriptorSetsCount = 0;

    XrBuffer *vertexBuffer = VK_NULL_HANDLE;
    XrBuffer *indexBuffer = VK_NULL_HANDLE;

    XrBuffer *uniformBuffers = VK_NULL_HANDLE;
    uint32_t uniformBuffersCount = 0;

    XrTexture *texture = VK_NULL_HANDLE;
    XrMaterial *material = VK_NULL_HANDLE;

    void (*draw)(XrUniformBuffer *ubo);
} XrModel;
