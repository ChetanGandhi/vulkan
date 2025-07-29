#pragma once

#include <xRenderer/platform.h>
#include <xRenderer/renderer.h>
#include <xRenderer/common.h>
#include <xRenderer/context.h>

void initializePlatformSpecificWindow();
void destroyPlatformSpecificWindow();

void initPlatformSpecificSurface(VkInstance *instance, VkSurfaceKHR *surface);
void destroyPlatformSpecificSurface();

void initializeVulkan();
void cleanUp();

int mainLoop();

void render(xr::UniformBufferObject *ubo);
void resize(uint32_t width, uint32_t height);
void toggleFullscreen(bool isFullscreen);

bool isRunning = true;
bool isFullscreen = false;
bool isEscapeKeyPressed = false;

std::string windowName;
std::string windowTitle;

xr::Context *context = nullptr;
xr::Renderer *renderer = nullptr;
