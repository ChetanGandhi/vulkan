#include "vulkanWindow.h"
#include "buildParam.h"
#include "renderer.h"
#include "utils.h"
#include <iostream>
#include <assert.h>

// Constructor

VulkanWindow::VulkanWindow(uint32_t width, uint32_t height, std::string name, std::string title)
{
    surfaceSize.width = width;
    surfaceSize.height = height;
    windowName = name;
    windowTitle = title;

    initPlatformSpecificWindow();

    this->renderer = new Renderer();

    initPlatformSpecificSurface();

    this->renderer->setSurface(surface);
    this->renderer->initDevice();
    this->renderer->initLogicalDevice();
    this->renderer->initSurface();
    this->renderer->initSwapchain();
    this->renderer->initSwapchainImages();
    this->renderer->initDepthStencilImage();
    this->renderer->initRenderPass();
    this->renderer->initFrameBuffers();
    this->renderer->initSynchronizations();
}

// Destructor

VulkanWindow::~VulkanWindow()
{
    vkQueueWaitIdle(this->renderer->getVulkanGraphicsQueue());
    this->renderer->destroySynchronizations();
    this->renderer->destroyFrameBuffers();
    this->renderer->destroyRenderPass();
    this->renderer->destoryDepthStencilImage();
    this->renderer->destroySwapchainImages();
    this->renderer->destroySwapchain();
    this->renderer->destroySurface();
    destroyPlatformSpecificSurface();

    this->renderer->destroyDevice();

    destroyPlatformSpecificWindow();
}

// Public methods

bool VulkanWindow::run()
{
    return update();
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

void VulkanWindow::beginRendering()
{
    renderer->beginRendering();
}

void VulkanWindow::endRendering(std::vector<VkSemaphore> waitSemaphores)
{
    renderer->endRendering(waitSemaphores);
}

VkExtent2D VulkanWindow::getVulkanSurfaceSize()
{
    VkExtent2D extent {};
    extent.width = surfaceSize.width;
    extent.height = surfaceSize.height;

    return extent;
}
