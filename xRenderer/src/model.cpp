#include "model.h"
#include "utils.h"

namespace xr
{
    Model::Model()
    {
    }

    Model::~Model()
    {
        vertices.clear();
        vertexIndices.clear();
    }
} // namespace xr
