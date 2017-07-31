#pragma once

#if defined (_WIN32) // check for Windows

#define VK_USE_PLATFORM_WIN32_KHR 1
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#include <windows.h>

#elif defined (__linux) // check for Linux

#define VK_USE_PLATFORM_XCB_KHR 1
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#include <xcb/xcb.h>

#else // platform not supported

#error Platform not suppotred yet.

#endif

#include <vulkan/vulkan.h>
