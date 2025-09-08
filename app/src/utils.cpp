#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define STB_IMAGE_IMPLEMENTATION

#include "lib/stb/stb_image.h"
#include "lib/tinyobj/tiny_obj_loader.h"

#include <xRenderer/logger.h>
#include <xRenderer/model.h>
#include <xRenderer/texture.h>
#include <xRenderer/vertex.h>

#include "utils.h"

bool xrLoadModal(const char *modelFilePath, XRModel *model)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;
    std::string warning;

    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, modelFilePath);

    if (!warning.empty())
    {
        XR_LOG_WARNING(warning.c_str());
    }

    if (!error.empty())
    {
        XR_LOG_ERROR(error.c_str());
    }

    if (!loaded)
    {
        XR_LOG_ERROR("Failed to load modal: %s | %s", modelFilePath, error.c_str());
        return false;
    }

    std::unordered_map<XRVertex, uint32_t> uniqueVertices = {};

    for (const tinyobj::shape_t &nextShape : shapes)
    {
        for (const tinyobj::index_t &nextIndex : nextShape.mesh.indices)
        {
            XRVertex nextVertex = {};

            // the attrib.vertices array is an array of float values instead of something like glm::vec3,
            // so you need to multiply the index by 3 to create group of 3 values.
            nextVertex.position = {
                attrib.vertices[3 * nextIndex.vertex_index + 0],
                attrib.vertices[3 * nextIndex.vertex_index + 1],
                attrib.vertices[3 * nextIndex.vertex_index + 2]
            };

            // the attrib.texcoords array is an array of float values instead of something like glm::vec2,
            // so you need to multiply the index by 2 to create group of 2 values.
            nextVertex.textureCoordinates = {attrib.texcoords[2 * nextIndex.texcoord_index + 0], 1.0 - attrib.texcoords[2 * nextIndex.texcoord_index + 1]};

            nextVertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(nextVertex) == 0)
            {
                uniqueVertices[nextVertex] = static_cast<uint32_t>(model->vertices.size());
                model->vertices.push_back(nextVertex);
            }

            model->vertexIndices.push_back(uniqueVertices[nextVertex]);
        }
    }

    return true;
};

void xrLoadTexture(const char *textureFilePath, XRTexture *texture, stbi_uc **pixels)
{
    int textureWidth = 0;
    int textureHeight = 0;
    int textureChannels = 0;

    *pixels = stbi_load(textureFilePath, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

    texture->width = textureWidth;
    texture->height = textureHeight;
    texture->channels = textureChannels;
    texture->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;
    texture->format = VK_FORMAT_R8G8B8A8_SRGB;
};
