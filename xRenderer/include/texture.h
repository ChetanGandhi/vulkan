#pragma once

#include "platform.h"
#include "common.h"

typedef struct XrTexture
{
    VkExtent2D extent;
    uint32_t channels;
    uint32_t mipLevels;
    VkFormat format;
} XrTexture;
