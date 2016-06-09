#pragma once

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1

#include <windows.h>

#else

#error Platform not suppotred yet.

#endif // _WIN32

#include <vulkan/vulkan.h>
