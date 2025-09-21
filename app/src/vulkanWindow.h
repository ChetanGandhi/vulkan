#pragma once

#ifndef NDEBUG

#define XR_ENABLE_RUNTIME_DEBUG 1
#define XR_ENABLE_DEBUG_REPORT_LOGGING 1
#define XR_ENABLE_DEBUG_REPORT_VERBOSE_BIT (XR_ENABLE_DEBUG_REPORT_LOGGING & 1)
#define XR_ENABLE_DEBUG_REPORT_INFORMATION_BIT (XR_ENABLE_DEBUG_REPORT_LOGGING & 1)
#define XR_ENABLE_FPS 1

#else

#define XR_ENABLE_RUNTIME_DEBUG 0
#define XR_ENABLE_DEBUG_REPORT_LOGGING 0
#define XR_ENABLE_DEBUG_REPORT_VERBOSE_BIT 0
#define XR_ENABLE_DEBUG_REPORT_INFORMATION_BIT 0
#define XR_ENABLE_FPS 0

#endif

#include <xRenderer/platform.h>
#include <xRenderer/renderer.h>
#include <xRenderer/common.h>
#include <xRenderer/context.h>

void initializePlatformSpecificWindow(XrContext *context);
void destroyPlatformSpecificWindow(XrContext *context);

VkResult initPlatformSpecificSurface(XrContext *context);
void destroyPlatformSpecificSurface(XrContext *context);

void listAllPhysicalDevices(XrContext *context, std::vector<XrPhysicalDevice> *gpuList);
void rankDevice(XrContext *context, XrPhysicalDevice *gpu);
void findSuitableDeviceQueues(XrContext *context, XrPhysicalDevice *gpu);
void findMaxMSAASampleCount(XrContext *context, XrPhysicalDevice *gpu);
bool checkDeviceExtensionSupport(XrContext *context, VkPhysicalDevice gpu);
bool hasStencilComponent(XrContext *context, VkFormat format);
void printGpuProperties(XrContext *context, XrPhysicalDevice *gpuDetails, uint32_t currentGpuIndex, uint32_t totalGpuCount);

VkResult initDevice(XrContext *context);
VkResult initializeVulkan(XrContext *context);
void cleanUp(XrContext *context);

int mainLoop(XrContext *context);

void render(XrContext *context, XrUniformBufferObject *ubo);
void resize(uint32_t width, uint32_t height);
void toggleFullscreen(bool isFullscreen);

bool isRunning = true;
bool isFullscreen = false;
bool isEscapeKeyPressed = false;

std::string windowName;
std::string windowTitle;

XrContext *context = VK_NULL_HANDLE;
