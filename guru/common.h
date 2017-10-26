#pragma once

struct QueueFamilyIndices {
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;
    uint32_t computeFamilyIndex = UINT32_MAX;
    bool hasSeparateComputeQueue = false;
    bool hasSeparatePresentQueue = false;
};

struct SurfaceSize {
    uint32_t width = 512;
    uint32_t height = 512;
};
