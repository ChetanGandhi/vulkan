#pragma once

#include "platform.h"
#include "renderer.h"
#include "common.h"
#include "logger.h"
#include "vulkanState.h"

int start();

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

xr::VulkanState *vkState = nullptr;
xr::Renderer *renderer = nullptr;

#if defined(VK_USE_PLATFORM_WIN32_KHR)

static uint64_t win32ClassIdCounter = 0;
std::string className;

HINSTANCE hGlobalInstance = NULL;
HWND hWindow = NULL;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

bool isActive = false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow);

#endif // VK_USE_PLATFORM_WIN32_KHR

#if defined(VK_USE_PLATFORM_XCB_KHR)

xcb_connection_t *xcbConnection = NULL;
xcb_screen_t *xcbScreen = NULL;
xcb_window_t xcbWindow;
xcb_intern_atom_reply_t *atom_wm_delete_window_reply = NULL;

bool isCloseButtonClicked = false;

int main(void);
void handleEvent(const xcb_generic_event_t *event);

#endif // VK_USE_PLATFORM_XCB_KHR
