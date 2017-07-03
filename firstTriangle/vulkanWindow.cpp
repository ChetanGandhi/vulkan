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

    this->renderer = new Renderer(surfaceSize);

    this->initPlatformSpecificSurface();

    this->renderer->setSurface(surface);
    this->renderer->initDevice();
    this->renderer->initLogicalDevice();
    this->renderer->initSwapchain();
    this->renderer->initSwapchainImageViews();
    // this->renderer->initDepthStencilImage();
    this->renderer->initRenderPass();
    this->renderer->initGraphicsPipline();
    this->renderer->initFrameBuffers();
    this->renderer->initCommandPool();
    this->renderer->initCommandBuffers();
    this->renderer->initSynchronizations();
}

// Destructor

VulkanWindow::~VulkanWindow()
{
    this->renderer->waitForIdle();
    this->renderer->destroySynchronizations();
    this->renderer->destroyCommandBuffers();
    this->renderer->destroyCommandPool();
    this->renderer->destroyFrameBuffers();
    this->renderer->destroyGraphicsPipline();
    this->renderer->destroyRenderPass();
    // this->renderer->destoryDepthStencilImage();
    this->renderer->destroySwapchainImageViews();
    this->renderer->destroySwapchain();

    this->destroyPlatformSpecificSurface();

    this->renderer->destroyDevice();
    delete this->renderer;

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

void VulkanWindow::render()
{
    renderer->render();
}

VkExtent2D VulkanWindow::getVulkanSurfaceSize()
{
    VkExtent2D extent {};
    extent.width = surfaceSize.width;
    extent.height = surfaceSize.height;

    return extent;
}
