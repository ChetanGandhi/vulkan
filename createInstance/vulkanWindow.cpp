#include "vulkanWindow.h"
#include "buildParam.h"
#include "Renderer.h"
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
}

VulkanWindow::~VulkanWindow()
{
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
