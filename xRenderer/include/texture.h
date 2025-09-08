#pragma once

#include "platform.h"
#include "common.h"

typedef struct XrTexture
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t mipLevels;
    VkFormat format;
} XrTexture;
