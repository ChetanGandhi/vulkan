#pragma once

#include <xRenderer/platform.h>
#include <xRenderer/renderer.h>
#include <xRenderer/common.h>
#include <xRenderer/context.h>

void initializePlatformSpecificWindow(XrContext *context);
void destroyPlatformSpecificWindow(XrContext *context);

VkResult initPlatformSpecificSurface(XrContext *context);
void destroyPlatformSpecificSurface(XrContext *context);

void initializeVulkan(XrContext *context);
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
