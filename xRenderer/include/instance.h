#pragma once

#include "platform.h"
#include "context.h"

VkResult xrCreateVulkanInstance(
    XrContext *context,
    VkApplicationInfo *applicationInfo,
    std::vector<const char *> *instanceLayers,
    std::vector<const char *> *instanceExtensions
);

VkResult xrDestroyVulkanInstance(XrContext *context);
