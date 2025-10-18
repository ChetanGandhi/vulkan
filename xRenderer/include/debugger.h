#pragma once

#include "platform.h"
#include "context.h"

XR_API VkResult xrCreateDebugger(XrContext *context, VkDebugReportFlagsEXT flags);
XR_API VkResult xrDestroyDebugger(XrContext *context);
