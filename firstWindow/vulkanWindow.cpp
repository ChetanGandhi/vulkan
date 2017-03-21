#include "vulkanWindow.h"
#include "buildParam.h"
#include "renderer.h"
#include "utils.h"
#include <iostream>
#include <assert.h>
#include <array>

// Constructor

VulkanWindow::VulkanWindow(Renderer *renderer, uint32_t width, uint32_t height, std::string name, std::string title)
{
    this->renderer = renderer;
    surfaceSize.width = width;
    surfaceSize.height = height;
    windowName = name;
    windowTitle = title;

    initPlatformSpecificWindow();
    initSurface();
    initSwapchain();
    initSwapchainImages();
    initDepthStencilImage();
    initRenderPass();
    initFrameBuffers();
    initSynchronizations();
}

// Destructor

VulkanWindow::~VulkanWindow()
{
    vkQueueWaitIdle(renderer->getVulkanQueue());
    destroySynchronizations();
    destroyFrameBuffers();
    destroyRenderPass();
    destoryDepthStencilImage();
    destroySwapchainImages();
    destroySwapchain();
    destroySurface();
    destroyPlatformSpecificWindow();
}

// Public methods

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
    VkResult result = vkAcquireNextImageKHR(renderer->getVulkanDevice(), swapchain, UINT64_MAX, VK_NULL_HANDLE, swapchainImageAvailable, &activeSwapchainImageId);

    checkError(result, __FILE__, __LINE__);

    result = vkWaitForFences(renderer->getVulkanDevice(), 1, &swapchainImageAvailable, VK_TRUE, UINT64_MAX);

    checkError(result, __FILE__, __LINE__);

    result = vkResetFences(renderer->getVulkanDevice(), 1, &swapchainImageAvailable);

    checkError(result, __FILE__, __LINE__);

    result = vkQueueWaitIdle(renderer->getVulkanQueue());

    checkError(result, __FILE__, __LINE__);
}

void VulkanWindow::endRendering(std::vector<VkSemaphore> waitSemaphores)
{
    VkResult presentResult = VkResult::VK_RESULT_MAX_ENUM;

    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = waitSemaphores.size();
    presentInfo.pWaitSemaphores = waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &activeSwapchainImageId;
    presentInfo.pResults = &presentResult;

    VkResult result = vkQueuePresentKHR(renderer->getVulkanQueue(), &presentInfo);

    checkError(result, __FILE__, __LINE__);
    checkError(presentResult, __FILE__, __LINE__);
}

VkRenderPass VulkanWindow::getVulkanRenderPass()
{
    return renderPass;
}

VkFramebuffer VulkanWindow::getVulkanActiveFramebuffer()
{
    return framebuffers[activeSwapchainImageId];
}

VkExtent2D VulkanWindow::getVulkanSurfaceSize()
{
    VkExtent2D extent {};
    extent.width = surfaceSize.width;
    extent.height = surfaceSize.height;

    return extent;
}

// Private methods

void VulkanWindow::initSurface()
{
    initPlatformSpecificSurface();

    VkPhysicalDevice gpu = renderer->getVulkanPhysicalDevice();

    VkBool32 isWSISupported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(gpu, renderer->getGraphicsFamilyIndex(), surface, &isWSISupported);

    if(!isWSISupported)
    {
        assert(0 && "WSI is not supported.");
        std::exit(-1);
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfaceCapabilities);

    if(surfaceCapabilities.currentExtent.width < UINT32_MAX)
    {
        surfaceSize.width = surfaceCapabilities.currentExtent.width;
        surfaceSize.height = surfaceCapabilities.currentExtent.height;
    }

    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);

        if(formatCount == 0)
        {
            assert(0 && "Surface format missing.");
            std::exit(-1);
        }

        std::vector<VkSurfaceFormatKHR> surfaceFormatList(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, surfaceFormatList.data());

        printSurfaceFormatsDetails(surfaceFormatList);

        if(surfaceFormatList[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.format = VK_FORMAT_B8G8R8_UNORM;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        }
        else
        {
            surfaceFormat = surfaceFormatList[0];
        }
    }
}

void VulkanWindow::destroySurface()
{
    destroyPlatformSpecificSurface();
}

void VulkanWindow::initSwapchain()
{

    // surfaceCapabilities.maxImageCount can be 0.
    // In this case the implementation supports unlimited amount of swap-chain images, limited by memory.
    // The amount of swap-chain images can also be fixed.
    if(swapchainImageCount < surfaceCapabilities.minImageCount + 1)
    {
        swapchainImageCount = surfaceCapabilities.minImageCount + 1;
    }

    if(swapchainImageCount > 0 && swapchainImageCount > surfaceCapabilities.maxImageCount)
    {
        swapchainImageCount = surfaceCapabilities.maxImageCount;
    }

    printSwapChainImageCount(surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount, swapchainImageCount);

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkResult result = VK_SUCCESS;

    {
        uint32_t presentModeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->getVulkanPhysicalDevice(), surface, &presentModeCount, nullptr);
        checkError(result, __FILE__, __LINE__);
        std::vector<VkPresentModeKHR> presentModeList(presentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->getVulkanPhysicalDevice(), surface, &presentModeCount, presentModeList.data());
        checkError(result, __FILE__, __LINE__);

        for(VkPresentModeKHR nextPresentMode : presentModeList)
        {
            if(nextPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = nextPresentMode;
            }
        }

        #if ENABLE_DEBUG

        std::cout<<"\n---------- Presentation Mode ----------\n";
        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            std::cout<<"\nMode: MAILBOX ["<<presentMode<<"]\n";
        }
        else
        {
            std::cout<<"\nMode: "<<presentMode<<"\n";
        }
        std::cout<<"\n---------- Presentation Mode End----------\n\n";

        #endif // ENABLE_DEBUG
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = swapchainImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent.width = surfaceSize.width;
    swapchainCreateInfo.imageExtent.height = surfaceSize.height;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(renderer->getVulkanDevice(), &swapchainCreateInfo, nullptr, &swapchain);
    checkError(result, __FILE__, __LINE__);

    result = vkGetSwapchainImagesKHR(renderer->getVulkanDevice(), swapchain, &swapchainImageCount, nullptr);
    checkError(result, __FILE__, __LINE__);
}

void VulkanWindow::destroySwapchain()
{
    vkDestroySwapchainKHR(renderer->getVulkanDevice(), swapchain, nullptr);
}

void VulkanWindow::initSwapchainImages()
{
    swapchainImages.resize(swapchainImageCount);
    swapchainImageViews.resize(swapchainImageCount);
    VkResult result = vkGetSwapchainImagesKHR(renderer->getVulkanDevice(), swapchain, &swapchainImageCount, swapchainImages.data());
    checkError(result, __FILE__, __LINE__);

    for(uint32_t counter = 0; counter < swapchainImageCount; ++counter)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = swapchainImages[counter];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(renderer->getVulkanDevice(), &imageViewCreateInfo, nullptr, &swapchainImageViews[counter]);
        checkError(result, __FILE__, __LINE__);
    }
}

void VulkanWindow::destroySwapchainImages()
{
    for(VkImageView imageView: swapchainImageViews)
    {
        vkDestroyImageView(renderer->getVulkanDevice(), imageView, nullptr);
    }
}

void VulkanWindow::initDepthStencilImage()
{
    {
        std::vector<VkFormat> formatsToTry
        {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D16_UNORM
        };

        for(VkFormat nextFormat : formatsToTry)
        {
            VkFormatProperties formatProperties {};
            vkGetPhysicalDeviceFormatProperties(renderer->getVulkanPhysicalDevice(), nextFormat, &formatProperties);

            if(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                depthStencilFormat = nextFormat;
                break;
            }
        }

        if(depthStencilFormat == VK_FORMAT_UNDEFINED)
        {
            assert(0 && "Depth stencil format not selected.");
            std::exit(-1);
        }

        stencilAvailable = (depthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT || depthStencilFormat == VK_FORMAT_S8_UINT) || false;
    }

    VkImageCreateInfo imageCreateInfo {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthStencilFormat;
    imageCreateInfo.extent.width = surfaceSize.width;
    imageCreateInfo.extent.height = surfaceSize.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(renderer->getVulkanDevice(), &imageCreateInfo, nullptr, &depthStencil.image);

    checkError(result, __FILE__, __LINE__);

    VkMemoryRequirements imageMemoryRequirements {};
    vkGetImageMemoryRequirements(renderer->getVulkanDevice(), depthStencil.image, &imageMemoryRequirements);

    VkPhysicalDeviceMemoryProperties gpuMemoryProperties = renderer->getVulkanPhysicalDeviceMemoryProperties();
    VkMemoryPropertyFlagBits requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    uint32_t memoryIndex = findMemoryTypeIndex(&gpuMemoryProperties, &imageMemoryRequirements, requiredMemoryProperties);

    VkMemoryAllocateInfo memoryAllocationInfo {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = nullptr;
    memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    result = vkAllocateMemory(renderer->getVulkanDevice(), &memoryAllocationInfo, nullptr, &depthStencil.imageMemory);

    checkError(result, __FILE__, __LINE__);

    result = vkBindImageMemory(renderer->getVulkanDevice(), depthStencil.image, depthStencil.imageMemory, 0);

    checkError(result, __FILE__, __LINE__);

    VkImageViewCreateInfo imageViewCreateInfo {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = depthStencil.image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = depthStencilFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (stencilAvailable ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(renderer->getVulkanDevice(), &imageViewCreateInfo, nullptr, &depthStencil.imageView);

    checkError(result, __FILE__, __LINE__);
}

void VulkanWindow::destoryDepthStencilImage()
{
    vkDestroyImageView(renderer->getVulkanDevice(), depthStencil.imageView, nullptr);
    vkFreeMemory(renderer->getVulkanDevice(), depthStencil.imageMemory, nullptr);
    vkDestroyImage(renderer->getVulkanDevice(), depthStencil.image, nullptr);
}

void VulkanWindow::initRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments {};
    // Depth and stencil attachment.
    attachments[0].flags = 0;
    attachments[0].format = depthStencilFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Color attachment.
    attachments[1].flags = 0;
    attachments[1].format = surfaceFormat.format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference subpassDepthStencilAttachment {};
    subpassDepthStencilAttachment.attachment = 0;
    subpassDepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 1> subpassColourAttachments {};
    subpassColourAttachments[0].attachment = 1;
    subpassColourAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDescription, 1> subpasses {};
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount = 0;
    subpasses[0].pInputAttachments = nullptr;
    subpasses[0].colorAttachmentCount = subpassColourAttachments.size();
    subpasses[0].pColorAttachments = subpassColourAttachments.data();
    subpasses[0].pResolveAttachments = nullptr;
    subpasses[0].pDepthStencilAttachment = &subpassDepthStencilAttachment;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPassCreateInfo {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = subpasses.size();
    renderPassCreateInfo.pSubpasses = subpasses.data();
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;

    VkResult result = vkCreateRenderPass(renderer->getVulkanDevice(), &renderPassCreateInfo, nullptr, &renderPass);

    checkError(result, __FILE__, __LINE__);
}

void VulkanWindow::destroyRenderPass()
{
    vkDestroyRenderPass(renderer->getVulkanDevice(), renderPass, nullptr);
}

void VulkanWindow::initFrameBuffers()
{
    framebuffers.resize(swapchainImageCount);

    for(uint32_t swapchainImageCounter = 0; swapchainImageCounter < swapchainImageCount; ++swapchainImageCounter)
    {
        std::array<VkImageView, 2> attachments {};
        attachments[0] = depthStencil.imageView;
        attachments[1] = swapchainImageViews[swapchainImageCounter];

        VkFramebufferCreateInfo framebufferCreateInfo {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = attachments.size();
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = surfaceSize.width;
        framebufferCreateInfo.height = surfaceSize.height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(renderer->getVulkanDevice(), &framebufferCreateInfo, nullptr, &framebuffers[swapchainImageCounter]);

        checkError(result, __FILE__, __LINE__);
    }
}

void VulkanWindow::destroyFrameBuffers()
{
    for(VkFramebuffer nextFrameuffer :framebuffers)
    {
        vkDestroyFramebuffer(renderer->getVulkanDevice(), nextFrameuffer, nullptr);
    }
}

void VulkanWindow::initSynchronizations()
{
    VkFenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = 0;

    VkResult result = vkCreateFence(renderer->getVulkanDevice(), &fenceCreateInfo, nullptr, &swapchainImageAvailable);

    checkError(result, __FILE__, __LINE__);
}

void VulkanWindow::destroySynchronizations()
{
    vkDestroyFence(renderer->getVulkanDevice(), swapchainImageAvailable, nullptr);
}

// Debug methods.

void VulkanWindow::printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
    #if ENABLE_DEBUG

    std::cout<<"\n---------- Surface Formats ----------\n";
    for(VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
    {
        std::cout<<"format\t\t: "<<nextSurfaceFormat.format<<"\n";
        std::cout<<"colorSpace\t: "<<nextSurfaceFormat.colorSpace<<"\n";
        std::cout<<"------------------------------------------------------------\n";
    }

    std::cout<<"\n---------- Surface Formats Details End ["<<surfaceFormats.size()<<"] ----------\n\n";

    #endif // ENABLE_DEBUG
}


void VulkanWindow::printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount)
{
    #if ENABLE_DEBUG

    std::cout<<"\n---------- Swapchain Image Count ----------\n";
    std::cout<<"Min\t: "<<minImageCount<<"\n";
    std::cout<<"Max\t: "<<maxImageCount<<"\n";
    std::cout<<"Current\t: "<<currentImageCount<<"\n";

    std::cout<<"\n---------- Swapchain Image Count End ----------\n\n";

    #endif // ENABLE_DEBUG
}
