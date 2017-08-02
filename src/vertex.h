#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 textureCoordinates;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
    {
        VkVertexInputAttributeDescription positionAttributeDescription = {};
        positionAttributeDescription.binding = 0;
        positionAttributeDescription.location = 0;
        positionAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        positionAttributeDescription.offset = offsetof(Vertex, position);

        VkVertexInputAttributeDescription colorAttributeDescription = {};
        colorAttributeDescription.binding = 0;
        colorAttributeDescription.location = 1;
        colorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        colorAttributeDescription.offset = offsetof(Vertex, color);

        VkVertexInputAttributeDescription textureCoordinatesAttributeDescription = {};
        textureCoordinatesAttributeDescription.binding = 0;
        textureCoordinatesAttributeDescription.location = 2;
        textureCoordinatesAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        textureCoordinatesAttributeDescription.offset = offsetof(Vertex, textureCoordinates);

        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {
            positionAttributeDescription,
            colorAttributeDescription,
            textureCoordinatesAttributeDescription
        };

        return attributeDescriptions;
    }

    bool operator==(const Vertex &otherVertex) const
    {
        return position == otherVertex.position && color == otherVertex.color && textureCoordinates == otherVertex.textureCoordinates;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.textureCoordinates) << 1);
        }
    };
}
