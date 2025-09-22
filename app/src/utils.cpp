#define TINYOBJLOADER_IMPLEMENTATION
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define STB_IMAGE_IMPLEMENTATION

#include "utils.h"

#include <xRenderer/logger.h>

void computeTangents(
    std::vector<tinyobj::index_t> *indices,
    std::vector<tinyobj::real_t> *vertices,
    std::vector<tinyobj::real_t> *uvs,
    std::vector<glm::vec3> *tangents
)
{
    // TANGENT

    glm::vec3 DeltaPos1;
    glm::vec3 DeltaPos2;
    glm::vec2 DeltaUV1;
    glm::vec2 DeltaUV2;
    glm::vec3 PerVerTangent;

    float r = 0.0f;
    uint32_t numberOfTriangles = (uint32_t)(indices->size() / 3);

    for (uint32_t index = 0; index < numberOfTriangles; ++index)
    {
        tinyobj::index_t index0 = (*indices)[index + 0];
        tinyobj::index_t index1 = (*indices)[index + 1];
        tinyobj::index_t index2 = (*indices)[index + 2];

        glm::vec3 vertex0 =
            glm::vec3((*vertices)[3 * index0.vertex_index + 0], (*vertices)[3 * index0.vertex_index + 1], (*vertices)[3 * index0.vertex_index + 2]);

        glm::vec3 vertex1 =
            glm::vec3((*vertices)[3 * index1.vertex_index + 0], (*vertices)[3 * index1.vertex_index + 1], (*vertices)[3 * index1.vertex_index + 2]);

        glm::vec3 vertex2 =
            glm::vec3((*vertices)[3 * index2.vertex_index + 0], (*vertices)[3 * index2.vertex_index + 1], (*vertices)[3 * index2.vertex_index + 2]);

        glm::vec2 uv0 = glm::vec2((*uvs)[2 * index0.texcoord_index + 0], (*uvs)[2 * index0.texcoord_index + 1]);
        glm::vec2 uv1 = glm::vec2((*uvs)[2 * index1.texcoord_index + 0], (*uvs)[2 * index1.texcoord_index + 1]);
        glm::vec2 uv2 = glm::vec2((*uvs)[2 * index2.texcoord_index + 0], (*uvs)[2 * index2.texcoord_index + 1]);

        DeltaPos1 = vertex1 - vertex0;
        DeltaPos2 = vertex2 - vertex0;

        DeltaUV1 = uv1 - uv0;
        DeltaUV2 = uv2 - uv0;

        if ((DeltaUV1[0] * DeltaUV2[1] - DeltaUV2[0] * DeltaUV1[1]) == 0.00f) // divide by zero
        {
            r = 1.0f;
        }
        else
        {
            r = 1.0f / (DeltaUV1[0] * DeltaUV2[1] - DeltaUV2[0] * DeltaUV1[1]);
        }

        PerVerTangent = (DeltaPos1 - DeltaPos2); // LHS-RHS

        for (int i = 0; i < 3; i++)
        {
            PerVerTangent[i] = r * (DeltaPos1[i] * DeltaUV2[1] - DeltaPos2[i] * DeltaUV1[1]);
        }

        (*tangents)[index + 0] += (PerVerTangent);
        (*tangents)[index + 1] += (PerVerTangent);
        (*tangents)[index + 2] += (PerVerTangent);
    }
}

bool xrLoadModal(XrContext *context, const char *modelFilePath, XrModel *model)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;
    std::string warning;

    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, modelFilePath);

    if (!warning.empty())
    {
        XR_LOG_WARNING(context->logger, warning.c_str());
    }

    if (!error.empty())
    {
        XR_LOG_ERROR(context->logger, error.c_str());
    }

    if (!loaded)
    {
        XR_LOG_ERROR(context->logger, "Failed to load modal: %s | %s", modelFilePath, error.c_str());
        return false;
    }

    std::unordered_map<XrVertex, uint32_t> uniqueVertices = {};
    std::vector<XrVertex> vertices = {};
    std::vector<uint32_t> vertexIndices = {};

    for (tinyobj::shape_t &nextShape : shapes)
    {
        std::vector<glm::vec3> tangent(nextShape.mesh.indices.size());
        computeTangents(&(nextShape.mesh.indices), &attrib.vertices, &attrib.texcoords, &tangent);

        for (uint32_t index = 0; index < nextShape.mesh.indices.size(); ++index)
        {
            tinyobj::index_t nextIndices = nextShape.mesh.indices[index];
            XrVertex nextVertex = {};

            // the attrib.vertices array is an array of float values instead of something like glm::vec3,
            // so you need to multiply the index by 3 to create group of 3 values.
            nextVertex.position = glm::vec3(
                attrib.vertices[3 * nextIndices.vertex_index + 0],
                attrib.vertices[3 * nextIndices.vertex_index + 1],
                attrib.vertices[3 * nextIndices.vertex_index + 2]
            );

            // the attrib.texcoords array is an array of float values instead of something like glm::vec2,
            // so you need to multiply the index by 2 to create group of 2 values.
            nextVertex.textureCoordinates =
                glm::vec2(attrib.texcoords[2 * nextIndices.texcoord_index + 0], 1.0f - attrib.texcoords[2 * nextIndices.texcoord_index + 1]);

            nextVertex.tangent = tangent[index];
            nextVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

            // the attrib.normals array is an array of float values instead of something like glm::vec3,
            // so you need to multiply the index by 3 to create group of 3 values.
            nextVertex.normal = glm::vec3(
                attrib.normals[3 * nextIndices.normal_index + 0],
                attrib.normals[3 * nextIndices.normal_index + 1],
                attrib.normals[3 * nextIndices.normal_index + 2]
            );

            if (uniqueVertices.count(nextVertex) == 0)
            {
                uniqueVertices[nextVertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(nextVertex);
            }

            vertexIndices.push_back(uniqueVertices[nextVertex]);
        }
    }

    model->vertices.assign(vertices.begin(), vertices.end());
    model->vertexIndices.assign(vertexIndices.begin(), vertexIndices.end());

    return true;
};

void xrLoadTexture(XrContext *context, const char *textureFilePath, XrTexture *texture, stbi_uc **pixels)
{
    int textureWidth = 0;
    int textureHeight = 0;
    int textureChannels = 0;

    *pixels = stbi_load(textureFilePath, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

    texture->extent.width = textureWidth;
    texture->extent.height = textureHeight;
    texture->channels = textureChannels;
    texture->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;
    texture->format = VK_FORMAT_R8G8B8A8_SRGB;
};
