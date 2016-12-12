#include "vulkanWindow.h"
#include "buildParam.h"
#include "Renderer.h"
#include "utils.h"
#include <iostream>
#include <assert.h>

VulkanWindow::VulkanWindow(Renderer *renderer, uint32_t sizeX, uint32_t sizeY, std::string name)
{
    this->renderer = renderer;
    surfaceSizeX = sizeX;
    surfaceSizeY = sizeY;
    windowName = name;

    initPlatformSpecificWindow();
    initSurface();
    initSwapchain();
    initSwapchainImages();
    initDepthStencilImage();
}

VulkanWindow::~VulkanWindow()
{
    destoryDepthStencilImage();
    destroySwapchainImages();
    destroySwapchain();
    destroySurface();
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
        surfaceSizeX = surfaceCapabilities.currentExtent.width;
        surfaceSizeY = surfaceCapabilities.currentExtent.height;
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
        checkError(result);
        std::vector<VkPresentModeKHR> presentModeList(presentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->getVulkanPhysicalDevice(), surface, &presentModeCount, presentModeList.data());
        checkError(result);

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
    swapchainCreateInfo.pNext = NULL;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = swapchainImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent.width = surfaceSizeX;
    swapchainCreateInfo.imageExtent.height = surfaceSizeY;
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
    checkError(result);

    result = vkGetSwapchainImagesKHR(renderer->getVulkanDevice(), swapchain, &swapchainImageCount, nullptr);
    checkError(result);
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
    checkError(result);

    for(uint32_t counter = 0; counter < swapchainImageCount; ++counter)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = NULL;
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
        checkError(result);
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

            if(depthStencilFormat == VK_FORMAT_UNDEFINED)
            {
                assert(0 && "Depth stencil format not selected.");
                std::exit(-1);
            }

            stencilAvailable = (depthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT || depthStencilFormat == VK_FORMAT_D32_SFLOAT || depthStencilFormat == VK_FORMAT_D16_UNORM);
        }
    }

    VkImageCreateInfo imageCreateInfo {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthStencilFormat;
    imageCreateInfo.extent.width = surfaceSizeX;
    imageCreateInfo.extent.height = surfaceSizeY;
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

    VkResult result = vkCreateImage(renderer->getVulkanDevice(), &imageCreateInfo, nullptr, &depthStencilImage);

    checkError(result);

    VkMemoryRequirements imageMemoryRequirements {};
    vkGetImageMemoryRequirements(renderer->getVulkanDevice(), depthStencilImage, &imageMemoryRequirements);

    VkPhysicalDeviceMemoryProperties gpuMemoryProperties = renderer->getVulkanPhysicalDeviceMemoryProperties();
    VkMemoryPropertyFlagBits requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    uint32_t memoryIndex = findMemoryTypeIndex(&gpuMemoryProperties, &imageMemoryRequirements, requiredMemoryProperties);

    VkMemoryAllocateInfo memoryAllocationInfo {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = NULL;
    memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    result = vkAllocateMemory(renderer->getVulkanDevice(), &memoryAllocationInfo, nullptr, &depthStencilImageMemory);

    checkError(result);

    result = vkBindImageMemory(renderer->getVulkanDevice(), depthStencilImage, depthStencilImageMemory, 0);

    checkError(result);

    VkImageViewCreateInfo imageViewCreateInfo {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = NULL;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = depthStencilImage;
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

    result = vkCreateImageView(renderer->getVulkanDevice(), &imageViewCreateInfo, nullptr, &depthStencilImageView);

    checkError(result);
}

void VulkanWindow::destoryDepthStencilImage()
{
    vkDestroyImageView(renderer->getVulkanDevice(), depthStencilImageView, nullptr);
    vkFreeMemory(renderer->getVulkanDevice(), depthStencilImageMemory, nullptr);
    vkDestroyImage(renderer->getVulkanDevice(), depthStencilImage, nullptr);
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
