#include "platform.h"
#include "vertex.h"

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

void xrGetBindingDescription(VkVertexInputBindingDescription *bindingDescription)
{
    bindingDescription->binding = 0;
    bindingDescription->stride = sizeof(XrVertex);
    bindingDescription->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void xrGetAttributeDescriptions(VkVertexInputAttributeDescription **attributeDescriptions, uint32_t *attributeDescriptionsCount)
{
    *attributeDescriptionsCount = 4;
    *attributeDescriptions = (VkVertexInputAttributeDescription *)malloc(sizeof(VkVertexInputAttributeDescription) * *attributeDescriptionsCount);
    memset((void *)*attributeDescriptions, 0, sizeof(VkVertexInputAttributeDescription) * (*attributeDescriptionsCount));

    // position;
    (*attributeDescriptions)[0].binding = 0;
    (*attributeDescriptions)[0].location = 0;
    (*attributeDescriptions)[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    (*attributeDescriptions)[0].offset = offsetof(XrVertex, position);

    // color
    (*attributeDescriptions)[1].binding = 0;
    (*attributeDescriptions)[1].location = 1;
    (*attributeDescriptions)[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    (*attributeDescriptions)[1].offset = offsetof(XrVertex, color);

    // texture coordinates;
    (*attributeDescriptions)[2].binding = 0;
    (*attributeDescriptions)[2].location = 2;
    (*attributeDescriptions)[2].format = VK_FORMAT_R32G32_SFLOAT;
    (*attributeDescriptions)[2].offset = offsetof(XrVertex, textureCoordinates);

    // normal = {};
    (*attributeDescriptions)[3].binding = 0;
    (*attributeDescriptions)[3].location = 3;
    (*attributeDescriptions)[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    (*attributeDescriptions)[3].offset = offsetof(XrVertex, normal);
}
