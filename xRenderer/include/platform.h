#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_CXX17

// Vulkan expects the data in your structure to be aligned in memory in a specific way
// We can use alignas(x) in structure before the variable deceleration
// but we can tell GLM to force alignment by this way.
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#if defined(_WIN32)
#ifdef XR_EXPORTS
#define XR_API __declspec(dllexport)
#else
#define XR_API __declspec(dllimport)
#endif
#elif defined(__linux)
#ifdef XR_EXPORTS
#define XR_API __attribute__((visibility("default")))
#else
#define XR_API
#endif
#elif
#define XR_API
#endif

#if defined(_WIN32) // check for Windows

#define VK_USE_PLATFORM_WIN32_KHR 1
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#include <windows.h>

#elif defined(__linux) // check for Linux

#define VK_USE_PLATFORM_XCB_KHR 1
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

#else // platform not supported

#error Platform not supported yet.

#endif

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>
#include <array>
#include <set>
#include <unordered_map>
#include <string>
#include <cstring>
#include <assert.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "buildParam.h"
#include "utils.h"
#include "logger.h"
