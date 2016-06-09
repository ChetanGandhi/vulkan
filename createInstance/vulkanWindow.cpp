#include "vulkanWindow.h"

VulkanWindow::VulkanWindow(uint32_t sizeX, uint32_t sizeY, std::string name)
{
    surfaceSizeX = sizeX;
    surfaceSizeY = sizeY;
    windowName = name;
    initPlatformSpecificWindow();
}

VulkanWindow::~VulkanWindow()
{
    destroyPlatformSpecificWindow();
}

void VulkanWindow::close()
{
    isRunning = false;
}

bool VulkanWindow::update()
{
    updatePlatformSpecificWindow();
    return isRunning;
}
