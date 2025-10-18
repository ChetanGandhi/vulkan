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

    bool operator==(const XrVertex &otherVertex) const
    {
        return position == otherVertex.position && color == otherVertex.color && textureCoordinates == otherVertex.textureCoordinates &&
               normal == otherVertex.normal;
    }
} XrVertex;

namespace std
{
    template <> struct hash<XrVertex>
    {
        size_t operator()(XrVertex const &vertex) const
        {
            size_t hashPosition = hash<glm::vec3>()(vertex.position);
            size_t hashColor = hash<glm::vec3>()(vertex.color);
            size_t hashNormals = hash<glm::vec3>()(vertex.normal);
            size_t hashtextureCoordinates = hash<glm::vec2>()(vertex.textureCoordinates);

            size_t combinedHash = hashPosition;
            combinedHash ^= (hashColor + (combinedHash << 1) + (combinedHash >> 1));
            combinedHash ^= (hashNormals + (combinedHash << 1) + (combinedHash >> 1));
            combinedHash ^= (hashtextureCoordinates + (combinedHash << 1) + (combinedHash >> 1));

            return combinedHash;
        }
    };
} // namespace std

void xrGetBindingDescription(VkVertexInputBindingDescription *bindingDescription);
void xrGetAttributeDescriptions(VkVertexInputAttributeDescription **attributeDescriptions, uint32_t *attributeDescriptionsCount);
