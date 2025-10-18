#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define STB_IMAGE_IMPLEMENTATION

#include "utils.h"

#include <xRenderer/common.h>
#include <xRenderer/vertex.h>
#include <xRenderer/logger.h>

void computeNormals(tinyobj::attrib_t *attrib, std::vector<tinyobj::shape_t> *shapes)
{
    // Step 1: Calculate face normals

    std::vector<glm::vec3> vertexNormals(attrib->vertices.size() / 3, glm::vec3(0.0f));

    for (uint32_t indexShapes = 0; indexShapes < shapes->size(); ++indexShapes)
    {
        tinyobj::shape_t *nextShape = &(*shapes)[indexShapes];
        size_t indexOffset = 0;

        for (size_t faceIndex = 0; faceIndex < nextShape->mesh.num_face_vertices.size(); ++faceIndex)
        {
            // As we are triangulating mesh
            tinyobj::index_t *vertex0 = &nextShape->mesh.indices[indexOffset + 0];
            tinyobj::index_t *vertex1 = &nextShape->mesh.indices[indexOffset + 1];
            tinyobj::index_t *vertex2 = &nextShape->mesh.indices[indexOffset + 2];

            glm::vec3 p0 = glm::vec3(
                attrib->vertices[3 * vertex0->vertex_index + 0],
                attrib->vertices[3 * vertex0->vertex_index + 1],
                attrib->vertices[3 * vertex0->vertex_index + 2]
            );

            glm::vec3 p1 = glm::vec3(
                attrib->vertices[3 * vertex1->vertex_index + 0],
                attrib->vertices[3 * vertex1->vertex_index + 1],
                attrib->vertices[3 * vertex1->vertex_index + 2]
            );

            glm::vec3 p2 = glm::vec3(
                attrib->vertices[3 * vertex2->vertex_index + 0],
                attrib->vertices[3 * vertex2->vertex_index + 1],
                attrib->vertices[3 * vertex2->vertex_index + 2]
            );

            glm::vec3 U = p1 - p0;
            glm::vec3 V = p2 - p0;
            glm::vec3 faceNormal = glm::cross(U, V);

            vertexNormals[vertex0->vertex_index] += faceNormal;
            vertexNormals[vertex1->vertex_index] += faceNormal;
            vertexNormals[vertex2->vertex_index] += faceNormal;

            // As we are triangulating mesh
            indexOffset += 3;
        }
    }

    // Step 2: Store the normals in tinyobj format
    attrib->normals.resize(vertexNormals.size() * 3);

    for (size_t index = 0; index < vertexNormals.size(); ++index)
    {
        attrib->normals[3 * index + 0] = vertexNormals[index].x;
        attrib->normals[3 * index + 1] = vertexNormals[index].y;
        attrib->normals[3 * index + 2] = vertexNormals[index].z;
    }

    // Step 3: Update the normal_index for each vertex

    if (attrib->normals.size() > 0)
    {
        for (uint32_t indexShapes = 0; indexShapes < shapes->size(); ++indexShapes)
        {
            tinyobj::shape_t *nextShape = &(*shapes)[indexShapes];

            size_t indexOffset = 0;

            for (size_t faceIndex = 0; faceIndex < nextShape->mesh.num_face_vertices.size(); ++faceIndex)
            {
                size_t verticesPerFace = nextShape->mesh.num_face_vertices[faceIndex];

                // Loop over vertices in the face.
                for (size_t vertexIndex = 0; vertexIndex < verticesPerFace; ++vertexIndex)
                {
                    // Get the vertex index from the current face.
                    tinyobj::index_t *nextIndices = &nextShape->mesh.indices[indexOffset + vertexIndex];

                    // Assign the normal index to be the same as the vertex index.
                    // This is only correct if the normals array is a 1-to-1 match
                    // with the vertices array, which our computation ensures.
                    nextIndices->normal_index = nextIndices->vertex_index;
                }

                indexOffset += verticesPerFace;
            }
        }
    }
}

VkBool32 xrLoadModal(XrContext *context, const char *baseDir, const char *modelFile, XrModel *model)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;
    std::string warning;

    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, modelFile, baseDir);

    if (!warning.empty())
    {
        XR_LOG_WARNING(context->logger, "Modal: %s [%s]", warning.c_str(), modelFile);
    }

    if (!error.empty())
    {
        XR_LOG_ERROR(context->logger, "Modal: %s [%s]", error.c_str(), modelFile);
    }

    if (!loaded)
    {
        XR_LOG_ERROR(context->logger, "Failed to load modal : %s | %s", error.c_str(), modelFile);
        return VK_FALSE;
    }

    std::unordered_map<XrVertex, uint32_t> uniqueVertices = {};
    std::vector<XrVertex> vertices = {};
    std::vector<uint32_t> vertexIndices = {};

    bool hasTexture = attrib.texcoords.size() > 0;
    bool hasNormal = attrib.normals.size() > 0;
    bool hasColors = attrib.colors.size() > 0;

    if (!hasNormal)
    {
        computeNormals(&attrib, &shapes);
        hasNormal = attrib.normals.size() > 0;
    }

    for (uint32_t indexShapes = 0; indexShapes < shapes.size(); ++indexShapes)
    {
        tinyobj::shape_t *nextShape = &shapes[indexShapes];

        for (uint32_t indexIndices = 0; indexIndices < nextShape->mesh.indices.size(); ++indexIndices)
        {
            tinyobj::index_t *nextIndices = &nextShape->mesh.indices[indexIndices];
            XrVertex nextVertex = {};

            // the attrib.vertices array is an array of float values instead of something like glm::vec3,
            // so you need to multiply the index by 3 to create group of 3 values.
            nextVertex.position = glm::vec3(
                attrib.vertices[3 * nextIndices->vertex_index + 0],
                attrib.vertices[3 * nextIndices->vertex_index + 1],
                attrib.vertices[3 * nextIndices->vertex_index + 2]
            );

            // the attrib.texcoords array is an array of float values instead of something like glm::vec2,
            // so you need to multiply the index by 2 to create group of 2 values.
            if (hasTexture && nextIndices->texcoord_index >= 0)
            {
                nextVertex.textureCoordinates =
                    glm::vec2(attrib.texcoords[2 * nextIndices->texcoord_index + 0], 1.0f - attrib.texcoords[2 * nextIndices->texcoord_index + 1]);
            }
            else
            {
                nextVertex.textureCoordinates = glm::vec2(0.0f, 0.0f);
            }

            if (hasColors)
            {
                nextVertex.color = glm::vec3(
                    attrib.colors[3 * nextIndices->vertex_index + 0],
                    attrib.colors[3 * nextIndices->vertex_index + 1],
                    attrib.colors[3 * nextIndices->vertex_index + 2]
                );
            }
            else
            {
                nextVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            // the attrib.normals array is an array of float values instead of something like glm::vec3,
            // so you need to multiply the index by 3 to create group of 3 values.
            if (hasNormal && nextIndices->normal_index >= 0)
            {
                nextVertex.normal = glm::vec3(
                    attrib.normals[3 * nextIndices->normal_index + 0],
                    attrib.normals[3 * nextIndices->normal_index + 1],
                    attrib.normals[3 * nextIndices->normal_index + 2]
                );
            }
            else
            {
                nextVertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
            }

            if (uniqueVertices.count(nextVertex) == 0)
            {
                uniqueVertices[nextVertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(nextVertex);
            }

            vertexIndices.push_back(uniqueVertices[nextVertex]);
        }
    }

    model->vertices = (XrVertex *)malloc(sizeof(XrVertex) * vertices.size());
    memset((void *)model->vertices, 0, sizeof(XrVertex) * vertices.size());
    memcpy(model->vertices, vertices.data(), sizeof(XrVertex) * vertices.size());
    model->verticesCount = (uint32_t)vertices.size();

    model->vertexIndices = (uint32_t *)malloc(sizeof(uint32_t) * vertexIndices.size());
    memset((void *)model->vertexIndices, 0, sizeof(uint32_t) * vertexIndices.size());
    memcpy(model->vertexIndices, vertexIndices.data(), sizeof(uint32_t) * vertexIndices.size());
    model->vertexIndicesCount = (uint32_t)vertexIndices.size();

    model->material = (XrMaterial *)malloc(sizeof(XrMaterial));
    memset((void *)model->material, 0, sizeof(XrMaterial));

    if (materials.size())
    {
        model->material->ambient = glm::vec4(materials[0].ambient[0], materials[0].ambient[1], materials[0].ambient[2], 1.0f);
        model->material->diffuse = glm::vec4(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2], 1.0f);
        model->material->specular = glm::vec4(materials[0].specular[0], materials[0].specular[1], materials[0].specular[2], 1.0f);
        model->material->shininess = materials[0].shininess;
    }
    else
    {
        model->material->ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        model->material->diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        model->material->specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        model->material->shininess = 32.0f;
    }

    return VK_TRUE;
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
