#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "lib/tinyobj/tiny_obj_loader.h"

#include "model.h"
#include "utils.h"

namespace xr
{
    Model::Model(const char *modelFilePath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string error;

        bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &error, modelFilePath);

        if (!loaded)
        {
            logf("Model load error: %s", error.c_str());
            assert(0 && "Not able to load model.");
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        for (const tinyobj::shape_t &nextShape : shapes)
        {
            for (const tinyobj::index_t &nextIndex : nextShape.mesh.indices)
            {
                Vertex nextVertex = {};
                nextVertex.position = { // the attrib.vertices array is an array of float values instead of something like glm::vec3,
                                        // so you need to multiply the index by 3 to create group of 3 values.
                                        attrib.vertices[3 * nextIndex.vertex_index + 0],
                                        attrib.vertices[3 * nextIndex.vertex_index + 1],
                                        attrib.vertices[3 * nextIndex.vertex_index + 2]

                };

                nextVertex.textureCoordinates = { // the attrib.texcoords array is an array of float values instead of something like glm::vec2,
                                                  // so you need to multiply the index by 2 to create group of 2 values.
                                                  attrib.texcoords[2 * nextIndex.texcoord_index + 0],
                                                  1.0 - attrib.texcoords[2 * nextIndex.texcoord_index + 1]
                };

                nextVertex.color = { 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(nextVertex) == 0)
                {
                    uniqueVertices[nextVertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(nextVertex);
                }

                vertexIndices.push_back(uniqueVertices[nextVertex]);
            }
        }
    }

    Model::~Model()
    {
        vertices.clear();
        vertexIndices.clear();
    }
} // namespace xr
