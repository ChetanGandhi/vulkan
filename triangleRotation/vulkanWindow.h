#pragma once

#include <string>
#include <vector>

#include "platform.h"
#include "renderer.h"
#include "common.h"
#include "logger.h"

int start();

void initPlatformSpecificWindow();
void destroyPlatformSpecificWindow();

void initPlatformSpecificSurface();
void destroyPlatformSpecificSurface();

void initilizeVulkan();
void cleanUp();

int mainLoop();

void render();
void resize(uint32_t width, uint32_t height);
void toggleFullscreen(bool isFullscreen);
void onEscapeKeyPressed();

bool isRunning = true;
bool isActive = false;
bool isFullscreen = false;
bool isEscapeKeyPressed = false;

std::string windowName;
std::string windowTitle;

Renderer *renderer = nullptr;
SurfaceSize surfaceSize;

VkSurfaceKHR surface = VK_NULL_HANDLE;

#if VK_USE_PLATFORM_WIN32_KHR

static uint64_t win32ClassIdCounter = 0;
std::string className;

HINSTANCE hGlobalInstance = NULL;
HWND hWindow = NULL;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInsatnce, LPSTR lpszCmdLine, int nCmdShow);

#endif // VK_USE_PLATFORM_WIN32_KHR
