#pragma once

#include "platform.h"
#include "context.h"

VkResult xrCreateVulkanInstance(
    XrContext *context,
    VkApplicationInfo *applicationInfo,
    char **instanceLayers,
    uint32_t instanceLayersCount,
    char **instanceExtensions,
    uint32_t instanceExtensionsCount
);

void xrDestroyVulkanInstance(XrContext *context);
