#pragma once

#include "platform.h"
#include "context.h"

XR_API VkResult xrIsValidationLayerSupport(XrContext *context, const char *validationLayerName);

XR_API VkResult xrCreateDebugger(XrContext *context);
XR_API VkResult xrDestroyDebugger(XrContext *context);
