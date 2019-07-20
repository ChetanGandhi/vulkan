#pragma once

#include "platform.h"

namespace xr {
    struct QueueFamilyIndices {
        uint32_t graphicsFamilyIndex = UINT32_MAX;
        uint32_t presentFamilyIndex = UINT32_MAX;
        bool hasSeparatePresentQueue = false;
    };

    struct SurfaceSize {
        uint32_t width = 512;
        uint32_t height = 512;
    };
}
