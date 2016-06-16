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
}

VulkanWindow::~VulkanWindow()
{
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
    if(swapchainImageCount < surfaceCapabilities.minImageCount + 1)
    {
        swapchainImageCount = surfaceCapabilities.minImageCount + 1;
    }

    if(swapchainImageCount > surfaceCapabilities.maxImageCount)
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
