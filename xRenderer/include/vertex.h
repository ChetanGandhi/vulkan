#pragma once

#include "platform.h"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

typedef struct XrVertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 textureCoordinates;
    glm::vec3 normal;
    glm::vec3 tangent;

    static VkVertexInputBindingDescription xrGetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(XrVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static void xrGetAttributeDescription(std::vector<VkVertexInputAttributeDescription> *attributeDescriptions)
    {
        VkVertexInputAttributeDescription positionAttributeDescription = {};
        positionAttributeDescription.binding = 0;
        positionAttributeDescription.location = 0;
        positionAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        positionAttributeDescription.offset = offsetof(XrVertex, position);

        VkVertexInputAttributeDescription colorAttributeDescription = {};
        colorAttributeDescription.binding = 0;
        colorAttributeDescription.location = 1;
        colorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        colorAttributeDescription.offset = offsetof(XrVertex, color);

        VkVertexInputAttributeDescription textureCoordinatesAttributeDescription = {};
        textureCoordinatesAttributeDescription.binding = 0;
        textureCoordinatesAttributeDescription.location = 2;
        textureCoordinatesAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        textureCoordinatesAttributeDescription.offset = offsetof(XrVertex, textureCoordinates);

        VkVertexInputAttributeDescription normalAttributeDescription = {};
        normalAttributeDescription.binding = 0;
        normalAttributeDescription.location = 3;
        normalAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        normalAttributeDescription.offset = offsetof(XrVertex, normal);

        VkVertexInputAttributeDescription tangentAttributeDescription = {};
        tangentAttributeDescription.binding = 0;
        tangentAttributeDescription.location = 4;
        tangentAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        tangentAttributeDescription.offset = offsetof(XrVertex, tangent);

        attributeDescriptions->push_back(positionAttributeDescription);
        attributeDescriptions->push_back(colorAttributeDescription);
        attributeDescriptions->push_back(textureCoordinatesAttributeDescription);
        attributeDescriptions->push_back(normalAttributeDescription);
        attributeDescriptions->push_back(tangentAttributeDescription);
    }

    bool operator==(const XrVertex &otherVertex) const

    {
        return position == otherVertex.position && color == otherVertex.color && textureCoordinates == otherVertex.textureCoordinates &&
               normal == otherVertex.normal && tangent == otherVertex.tangent;
    }
} XrVertex;

namespace std
{
    template <> struct hash<XrVertex>
    {
        size_t operator()(XrVertex const &vertex) const
        {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.textureCoordinates) << 1);
        }
    };
} // namespace std
