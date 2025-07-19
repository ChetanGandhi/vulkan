#include "renderer.h"
#include "lib/stb/stb_image.h"
#include "lib/tinyobj/tiny_obj_loader.h"
#include "utils.h"
#include "logger.h"

namespace xr
{
    XR_API Renderer::Renderer(VulkanState *vkState)
    {
        this->vkState = vkState;
        this->vkState->debugger = new Debugger();
        setupLayersAndExtensions();
        initInstance();
    }

    XR_API Renderer::~Renderer()
    {
        this->vkState->vertices.clear();
        this->vkState->vertexIndices.clear();

        destroyInstance();

        this->vkState = nullptr;
    }

    void Renderer::setupLayersAndExtensions()
    {
        this->vkState->instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        this->vkState->instanceExtensions.push_back(PLATFORM_SURFACE_EXTENSION_NAME);
        this->vkState->instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        this->vkState->deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    XR_API void Renderer::initInstance()
    {
        VkApplicationInfo applicationInfo = {};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pNext = nullptr;
        applicationInfo.apiVersion = VK_API_VERSION_1_0;
        applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.pApplicationName = "Vulkan";
        applicationInfo.pEngineName = nullptr;
        applicationInfo.engineVersion = 0;

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {};
        this->vkState->debugger->fillCreateInfo(&debugUtilsMessengerCreateInfo);

        if (this->vkState->debugger->checkValidationLayerSupport())
        {
            this->vkState->instanceLayers.push_back(this->vkState->debugger->validationLayerName);
        }

        this->vkState->instance = new Instance();
        VkResult result = this->vkState->instance->initVulkanInstance(
            &applicationInfo, &(this->vkState->instanceLayers), &(this->vkState->instanceExtensions), &debugUtilsMessengerCreateInfo
        );
        CHECK_ERROR(result);

        this->vkState->debugger->initialize(&(this->vkState->instance->vkInstance), &debugUtilsMessengerCreateInfo);
    }

    XR_API void Renderer::destroyInstance()
    {
        this->vkState->debugger->destory(&(this->vkState->instance->vkInstance));
        delete this->vkState->debugger;
        this->vkState->debugger = nullptr;

        delete this->vkState->instance;
        this->vkState->instance = nullptr;
    }

    XR_API void Renderer::waitForIdle()
    {
        vkQueueWaitIdle(this->vkState->graphicsQueue);

        if (this->vkState->queueFamilyIndices.hasSeparatePresentQueue)
        {
            vkQueueWaitIdle(this->vkState->presentQueue);
        }

        vkDeviceWaitIdle(this->vkState->device);
    }

    void Renderer::listAllPhysicalDevices(std::vector<GpuDetails> *gpuDetailsList)
    {
        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(this->vkState->instance->vkInstance, &gpuCount, VK_NULL_HANDLE);

        if (gpuCount == 0)
        {
            return;
        }

        std::vector<VkPhysicalDevice> deviceList(gpuCount);
        vkEnumeratePhysicalDevices(this->vkState->instance->vkInstance, &gpuCount, deviceList.data());

        for (uint32_t counter = 0; counter < gpuCount; ++counter)
        {
            VkPhysicalDevice nextGpu = deviceList[counter];
            VkPhysicalDeviceProperties nextGpuProperties = {};
            VkPhysicalDeviceMemoryProperties nextGpuMemoryProperties = {};

            vkGetPhysicalDeviceProperties(nextGpu, &nextGpuProperties);
            vkGetPhysicalDeviceMemoryProperties(nextGpu, &nextGpuMemoryProperties);

            GpuDetails nextPhysicalDevice = {};
            nextPhysicalDevice.gpu = nextGpu;
            nextPhysicalDevice.properties = nextGpuProperties;
            nextPhysicalDevice.memoryProperties = nextGpuMemoryProperties;
            gpuDetailsList->push_back(nextPhysicalDevice);
        }
    }

    bool Renderer::isDeviceSuitable(VkPhysicalDevice gpu)
    {
        QueueFamilyIndices indices = {};

        bool suitableDeviceQueuesFound = findSuitableDeviceQueues(gpu, &indices);

        if (suitableDeviceQueuesFound)
        {
            this->vkState->queueFamilyIndices.graphicsFamilyIndex = indices.graphicsFamilyIndex;
            this->vkState->queueFamilyIndices.presentFamilyIndex = indices.presentFamilyIndex;
            this->vkState->queueFamilyIndices.hasSeparatePresentQueue = indices.hasSeparatePresentQueue;

            logf("---------- Queue Family Indices ----------");
            logf("Graphics Family Index\t\t: %d", this->vkState->queueFamilyIndices.graphicsFamilyIndex);
            logf("Present Family Index\t\t: %d", this->vkState->queueFamilyIndices.presentFamilyIndex);
            logf("Has Separate Present Queue\t: %d", this->vkState->queueFamilyIndices.hasSeparatePresentQueue);
            logf("---------- Queue Family Indices End ----------");
        }

        bool extensionSupported = checkDeviceExtensionSupport(gpu);
        bool swapchainSupported = true;

        if (extensionSupported)
        {
            SwapchainSupportDetails details = {};
            querySwapchainSupportDetails(gpu, &details);
            swapchainSupported = !details.surfaceFormats.empty() && !details.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures = {};
        vkGetPhysicalDeviceFeatures(gpu, &supportedFeatures);

        return suitableDeviceQueuesFound && extensionSupported && swapchainSupported && supportedFeatures.samplerAnisotropy;
    }

    bool Renderer::findSuitableDeviceQueues(VkPhysicalDevice gpu, QueueFamilyIndices *queueFamilyIndices)
    {
        uint32_t familyCount = 0;
        uint32_t graphicsFamilyIndex = UINT32_MAX;
        uint32_t presentFamilyIndex = UINT32_MAX;

        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, familyPropertiesList.data());

        for (uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
        {
            const VkQueueFamilyProperties nextFamilyProperties = familyPropertiesList[queueCounter];

            if (nextFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsFamilyIndex = queueCounter;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queueCounter, this->vkState->surface, &presentSupport);

            if (presentSupport == VK_TRUE)
            {
                graphicsFamilyIndex = queueCounter;
                presentFamilyIndex = queueCounter;
                break;
            }
        }

        if (presentFamilyIndex == UINT32_MAX)
        {
            for (uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
            {
                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queueCounter, this->vkState->surface, &presentSupport);

                if (presentSupport == VK_TRUE)
                {
                    presentFamilyIndex = queueCounter;
                    break;
                }
            }
        }

        if (graphicsFamilyIndex == UINT32_MAX || presentFamilyIndex == UINT32_MAX)
        {
            return false;
        }

        queueFamilyIndices->graphicsFamilyIndex = graphicsFamilyIndex;
        queueFamilyIndices->presentFamilyIndex = presentFamilyIndex;
        queueFamilyIndices->hasSeparatePresentQueue = (presentFamilyIndex != graphicsFamilyIndex);

        return true;
    }

    VkSampleCountFlagBits Renderer::findMaxMSAASampleCount(VkPhysicalDeviceProperties properties)
    {
        VkSampleCountFlags sampleCountFlags = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        if (sampleCountFlags & VK_SAMPLE_COUNT_64_BIT)
        {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (sampleCountFlags & VK_SAMPLE_COUNT_32_BIT)
        {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (sampleCountFlags & VK_SAMPLE_COUNT_16_BIT)
        {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (sampleCountFlags & VK_SAMPLE_COUNT_8_BIT)
        {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (sampleCountFlags & VK_SAMPLE_COUNT_4_BIT)
        {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (sampleCountFlags & VK_SAMPLE_COUNT_2_BIT)
        {
            return VK_SAMPLE_COUNT_2_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice gpu)
    {
        uint32_t availableDeviceExtensionsCount = 0;

        VkResult result = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, nullptr);

        CHECK_ERROR(result);

        if (availableDeviceExtensionsCount == 0)
        {
            return false;
        }

        std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionsCount);
        result = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, availableDeviceExtensions.data());

        CHECK_ERROR(result);

        std::set<std::string> requiredExtensions(this->vkState->deviceExtensions.begin(), this->vkState->deviceExtensions.end());

        for (const VkExtensionProperties &nextExtensionProperties : availableDeviceExtensions)
        {
            requiredExtensions.erase(nextExtensionProperties.extensionName);
        }

        return requiredExtensions.empty();
    }

    XR_API void Renderer::initDevice()
    {
        {
            std::vector<GpuDetails> gpuDetailsList(0);
            listAllPhysicalDevices(&gpuDetailsList);

            uint32_t gpuCount = static_cast<uint32_t>(gpuDetailsList.size());
            uint32_t selectedGpuIndex = 0;

            logf("---------- Total GPU Found [%d]----------", gpuCount);

            for (uint32_t counter = 0; counter < gpuCount; ++counter)
            {
                GpuDetails nextGpuDetails = gpuDetailsList[counter];
                printGpuProperties(&nextGpuDetails.properties, counter + 1, gpuCount);
            }

            for (uint32_t counter = 0; counter < gpuCount; ++counter)
            {
                GpuDetails nextGpuDetails = gpuDetailsList[counter];

                if (isDeviceSuitable(nextGpuDetails.gpu))
                {
                    this->vkState->gpuDetails = nextGpuDetails;
                    this->vkState->msaaSamples = findMaxMSAASampleCount(nextGpuDetails.properties);
                    selectedGpuIndex = counter;
                    break;
                }
            }

            if (this->vkState->gpuDetails.gpu == VK_NULL_HANDLE)
            {
                assert(0 && "Vulkan Error: Queue family supporting graphics device not found.");
                std::exit(EXIT_FAILURE);
            }

            logf("---------- Selected GPU Properties ----------");
            printGpuProperties(&(this->vkState->gpuDetails.properties), (selectedGpuIndex + 1), gpuCount);
            logf("---------- Selected GPU Properties End ----------");

            logf("---------- MSAA Count ----------");
            logf("MSAA samples count: %d", this->vkState->msaaSamples);
            logf("---------- MSAA Count End ----------");
        }

        {
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);
            std::vector<VkLayerProperties> layerPropertiesList(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, layerPropertiesList.data());
            printInstanceLayerProperties(layerPropertiesList);
        }

        {
            uint32_t layerCount = 0;
            vkEnumerateDeviceLayerProperties(this->vkState->gpuDetails.gpu, &layerCount, VK_NULL_HANDLE);
            std::vector<VkLayerProperties> layerPropertiesList(layerCount);
            vkEnumerateDeviceLayerProperties(this->vkState->gpuDetails.gpu, &layerCount, layerPropertiesList.data());
            printDeviceLayerProperties(layerPropertiesList);
        }
    }

    XR_API void Renderer::initLogicalDevice()
    {
        std::vector<float> queuePriorities = { 0.0f };
        std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos(0);

        VkDeviceQueueCreateInfo deviceGraphicQueueCreateInfo = {};
        deviceGraphicQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceGraphicQueueCreateInfo.pNext = nullptr;
        deviceGraphicQueueCreateInfo.flags = 0;
        deviceGraphicQueueCreateInfo.queueFamilyIndex = this->vkState->queueFamilyIndices.graphicsFamilyIndex;
        deviceGraphicQueueCreateInfo.queueCount = 1;
        deviceGraphicQueueCreateInfo.pQueuePriorities = queuePriorities.data();

        deviceQueueCreateInfos.push_back(deviceGraphicQueueCreateInfo);

        if (this->vkState->queueFamilyIndices.hasSeparatePresentQueue)
        {
            VkDeviceQueueCreateInfo devicePresentQueueCreateInfo = {};
            devicePresentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            devicePresentQueueCreateInfo.pNext = nullptr;
            devicePresentQueueCreateInfo.flags = 0;
            devicePresentQueueCreateInfo.queueFamilyIndex = this->vkState->queueFamilyIndices.graphicsFamilyIndex;
            devicePresentQueueCreateInfo.queueCount = 1;
            devicePresentQueueCreateInfo.pQueuePriorities = queuePriorities.data();

            deviceQueueCreateInfos.push_back(devicePresentQueueCreateInfo);
        }

        // As we are using texture sampler, we need to enable this as a device feature.
        // This have many VkBool32 properties, leave it to VK_FALSE right now.
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(this->vkState->deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = this->vkState->deviceExtensions.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        VkResult result = vkCreateDevice(this->vkState->gpuDetails.gpu, &deviceCreateInfo, VK_NULL_HANDLE, &(this->vkState->device));
        CHECK_ERROR(result);

        // Create the graphic queue using graphicsFamilyIndex for given physical device.
        vkGetDeviceQueue(this->vkState->device, this->vkState->queueFamilyIndices.graphicsFamilyIndex, 0, &(this->vkState->graphicsQueue));

        if (!this->vkState->queueFamilyIndices.hasSeparatePresentQueue)
        {
            this->vkState->presentQueue = this->vkState->graphicsQueue;
        }
        else
        {
            vkGetDeviceQueue(this->vkState->device, this->vkState->queueFamilyIndices.presentFamilyIndex, 0, &(this->vkState->presentQueue));
        }
    }

    XR_API void Renderer::destroyDevice()
    {
        vkDestroyDevice(this->vkState->device, VK_NULL_HANDLE);
        this->vkState->device = VK_NULL_HANDLE;
    }

    void Renderer::querySwapchainSupportDetails(VkPhysicalDevice gpu, SwapchainSupportDetails *details)
    {
        VkResult result = VK_SUCCESS;

        uint32_t formatCount = 0;
        uint32_t presentModeCount = 0;

        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, this->vkState->surface, &(details->surfaceCapabilities));
        CHECK_ERROR(result);

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, this->vkState->surface, &formatCount, nullptr);
        CHECK_ERROR(result);

        details->surfaceFormats.resize(formatCount);

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, this->vkState->surface, &formatCount, details->surfaceFormats.data());
        CHECK_ERROR(result);

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, this->vkState->surface, &presentModeCount, nullptr);
        CHECK_ERROR(result);

        details->presentModes.resize(presentModeCount);

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, this->vkState->surface, &presentModeCount, details->presentModes.data());
        CHECK_ERROR(result);
    }

    VkSurfaceFormatKHR Renderer::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats)
    {
        printSurfaceFormatsDetails(surfaceFormats);

        if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            VkSurfaceFormatKHR surfaceFormat = {};
            surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            return surfaceFormat;
        }

        for (const VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
        {
            if (nextSurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && nextSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return nextSurfaceFormat;
            }
        }

        return surfaceFormats[0];
    }

    VkPresentModeKHR Renderer::choosePresentMode(const std::vector<VkPresentModeKHR> &presentModes)
    {
        VkPresentModeKHR defaultPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        for (const VkPresentModeKHR &nextPresentMode : presentModes)
        {
            // If nextPresentMode is VK_PRESENT_MODE_MAILBOX_KHR then use this as this is the best.
            if (nextPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return nextPresentMode;
            }

            // If VK_PRESENT_MODE_MAILBOX_KHR was not found then use VK_PRESENT_MODE_IMMEDIATE_KHR.
            if (nextPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                defaultPresentMode = nextPresentMode;
            }
        }

        return defaultPresentMode;
    }

    void Renderer::chooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent)
    {
        if (surfaceCapabilities.currentExtent.width < UINT32_MAX)
        {
            initialSurfaceExtent->width = surfaceCapabilities.currentExtent.width;
            initialSurfaceExtent->height = surfaceCapabilities.currentExtent.height;
        }
        else
        {
            if (initialSurfaceExtent->width > surfaceCapabilities.maxImageExtent.width)
            {
                initialSurfaceExtent->width = surfaceCapabilities.maxImageExtent.width;
            }

            if (initialSurfaceExtent->width < surfaceCapabilities.minImageExtent.width)
            {
                initialSurfaceExtent->width = surfaceCapabilities.minImageExtent.width;
            }

            if (initialSurfaceExtent->height > surfaceCapabilities.maxImageExtent.height)
            {
                initialSurfaceExtent->height = surfaceCapabilities.maxImageExtent.height;
            }

            if (initialSurfaceExtent->height < surfaceCapabilities.minImageExtent.height)
            {
                initialSurfaceExtent->height = surfaceCapabilities.minImageExtent.height;
            }
        }
    }

    XR_API void Renderer::initSwapchain()
    {
        VkExtent2D initialSurfaceExtent = {};
        initialSurfaceExtent.width = this->vkState->surfaceSize.width;
        initialSurfaceExtent.height = this->vkState->surfaceSize.height;

        querySwapchainSupportDetails(this->vkState->gpuDetails.gpu, &(this->vkState->swapchainSupportDetails));

        if (!this->vkState->swapchainSupportDetails.surfaceFormats.size())
        {
            assert(0 && "Surface format missing.");
            std::exit(EXIT_FAILURE);
        }

        this->vkState->surfaceFormat = chooseSurfaceFormat(this->vkState->swapchainSupportDetails.surfaceFormats);
        chooseSurfaceExtent(this->vkState->swapchainSupportDetails.surfaceCapabilities, &initialSurfaceExtent);

        this->vkState->surfaceSize.width = initialSurfaceExtent.width;
        this->vkState->surfaceSize.height = initialSurfaceExtent.height;

        VkPresentModeKHR presentMode = choosePresentMode(this->vkState->swapchainSupportDetails.presentModes);

        // surfaceCapabilities.maxImageCount can be 0.
        // In this case the implementation supports unlimited amount of swap-chain images, limited by memory.
        // The amount of swap-chain images can also be fixed.
        if (this->vkState->swapchainImageCount < this->vkState->swapchainSupportDetails.surfaceCapabilities.minImageCount + 1)
        {
            this->vkState->swapchainImageCount = this->vkState->swapchainSupportDetails.surfaceCapabilities.minImageCount + 1;
        }

        if (this->vkState->swapchainImageCount > 0 &&
            this->vkState->swapchainImageCount > this->vkState->swapchainSupportDetails.surfaceCapabilities.maxImageCount)
        {
            this->vkState->swapchainImageCount = this->vkState->swapchainSupportDetails.surfaceCapabilities.maxImageCount;
        }

        printSwapChainImageCount(
            this->vkState->swapchainSupportDetails.surfaceCapabilities.minImageCount,
            this->vkState->swapchainSupportDetails.surfaceCapabilities.maxImageCount,
            this->vkState->swapchainImageCount
        );

        {
            logf("---------- Presentation Mode ----------");

            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                logf("Mode: MAILBOX [%d]", presentMode);
            }
            else
            {
                logf("Mode: %d", presentMode);
            }

            logf("---------- Presentation Mode End----------");
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = nullptr;
        swapchainCreateInfo.flags = 0;
        swapchainCreateInfo.surface = this->vkState->surface;
        swapchainCreateInfo.minImageCount = this->vkState->swapchainImageCount;
        swapchainCreateInfo.imageFormat = this->vkState->surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = this->vkState->surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent.width = this->vkState->surfaceSize.width;
        swapchainCreateInfo.imageExtent.height = this->vkState->surfaceSize.height;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.preTransform = this->vkState->swapchainSupportDetails.surfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (this->vkState->queueFamilyIndices.hasSeparatePresentQueue)
        {
            std::vector<uint32_t> indices = { this->vkState->queueFamilyIndices.graphicsFamilyIndex, this->vkState->queueFamilyIndices.presentFamilyIndex };

            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size()); // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
            swapchainCreateInfo.pQueueFamilyIndices = indices.data();
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0;     // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
            swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        }

        VkResult result = vkCreateSwapchainKHR(this->vkState->device, &swapchainCreateInfo, nullptr, &(this->vkState->swapchain));
        CHECK_ERROR(result);

        result = vkGetSwapchainImagesKHR(this->vkState->device, this->vkState->swapchain, &(this->vkState->swapchainImageCount), nullptr);
        CHECK_ERROR(result);

        this->vkState->swapchainImages.resize(this->vkState->swapchainImageCount);
        result = vkGetSwapchainImagesKHR(
            this->vkState->device, this->vkState->swapchain, &(this->vkState->swapchainImageCount), this->vkState->swapchainImages.data()
        );
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destroySwapchain()
    {
        vkDestroySwapchainKHR(this->vkState->device, this->vkState->swapchain, nullptr);
        this->vkState->swapchain = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initSwapchainImageViews()
    {
        this->vkState->swapchainImageViews.resize(this->vkState->swapchainImageCount);

        for (uint32_t counter = 0; counter < this->vkState->swapchainImageCount; ++counter)
        {
            createImageView(
                this->vkState->swapchainImages[counter],
                this->vkState->surfaceFormat.format,
                this->vkState->swapchainImageViews[counter],
                VK_IMAGE_ASPECT_COLOR_BIT,
                1
            );
        }
    }

    XR_API void Renderer::destroySwapchainImageViews()
    {
        for (VkImageView imageView : this->vkState->swapchainImageViews)
        {
            vkDestroyImageView(this->vkState->device, imageView, nullptr);
        }

        this->vkState->swapchainImageViews.clear();
    }

    XR_API VkShaderModule Renderer::createShaderModule(const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.pNext = nullptr;
        shaderModuleCreateInfo.flags = 0;
        shaderModuleCreateInfo.codeSize = code.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(this->vkState->device, &shaderModuleCreateInfo, nullptr, &shaderModule);
        CHECK_ERROR(result);

        return shaderModule;
    }

    XR_API void Renderer::initGraphicsPiplineCache()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCreateInfo.pNext = NULL;
        pipelineCacheCreateInfo.flags = 0;
        pipelineCacheCreateInfo.initialDataSize = 0;
        pipelineCacheCreateInfo.pInitialData = NULL;

        VkResult result = vkCreatePipelineCache(this->vkState->device, &pipelineCacheCreateInfo, nullptr, &(this->vkState->pipelineCache));
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destroyGraphicsPiplineCache()
    {
        vkDestroyPipelineCache(this->vkState->device, this->vkState->pipelineCache, nullptr);
        this->vkState->pipelineCache = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initGraphicsPipline()
    {
        std::vector<char> vertexShaderCode;
        std::vector<char> fragmentShaderCode;

        if (!readFile(this->vkState->vertexShaderFilePath, &vertexShaderCode))
        {
            logf("Cannot open vertex shader file: %s", this->vkState->vertexShaderFilePath);
            assert(0 && "Cannot open vertex shader.");
        }

        if (!readFile(this->vkState->fragmentShaderFile, &fragmentShaderCode))
        {
            logf("Cannot open fragment shader file: %s", this->vkState->fragmentShaderFile);
            assert(0 && "Cannot open fragment shader.");
        }

        VkShaderModule vertexShaderModule = this->createShaderModule(vertexShaderCode);
        VkShaderModule fragmentShaderModule = this->createShaderModule(fragmentShaderCode);

        VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
        vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageCreateInfo.pNext = nullptr;
        vertexShaderStageCreateInfo.flags = 0;
        vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageCreateInfo.module = vertexShaderModule;
        vertexShaderStageCreateInfo.pName = "main";
        vertexShaderStageCreateInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
        fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageCreateInfo.pNext = nullptr;
        fragmentShaderStageCreateInfo.flags = 0;
        fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageCreateInfo.module = fragmentShaderModule;
        fragmentShaderStageCreateInfo.pName = "main";
        fragmentShaderStageCreateInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

        VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
        std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescription = Vertex::getAttributeDescription();

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.pNext = nullptr;
        vertexInputStateCreateInfo.flags = 0;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.pNext = nullptr;
        inputAssemblyStateCreateInfo.flags = 0;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)this->vkState->surfaceSize.width;
        viewport.height = (float)this->vkState->surfaceSize.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent.width = this->vkState->surfaceSize.width;
        scissor.extent.height = this->vkState->surfaceSize.height;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.pNext = nullptr;
        viewportStateCreateInfo.flags = 0;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.pNext = nullptr;
        rasterizationStateCreateInfo.flags = 0;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
        rasterizationStateCreateInfo.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.pNext = nullptr;
        multisampleStateCreateInfo.flags = 0;
        multisampleStateCreateInfo.rasterizationSamples = this->vkState->msaaSamples;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.minSampleShading = 1.0f;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.pNext = nullptr;
        depthStencilStateCreateInfo.flags = 0;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.front = {};
        depthStencilStateCreateInfo.back = {};
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendingStateCreateInfo = {};
        colorBlendingStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingStateCreateInfo.pNext = nullptr;
        colorBlendingStateCreateInfo.flags = 0;
        colorBlendingStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendingStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendingStateCreateInfo.attachmentCount = 1;
        colorBlendingStateCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendingStateCreateInfo.blendConstants[0] = 0.0f;
        colorBlendingStateCreateInfo.blendConstants[1] = 0.0f;
        colorBlendingStateCreateInfo.blendConstants[2] = 0.0f;
        colorBlendingStateCreateInfo.blendConstants[3] = 0.0f;

        // We are not yet using the dynamic state hence removing this for now.
        // std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
        std::vector<VkDynamicState> dynamicStates = {};

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.pNext = nullptr;
        dynamicStateCreateInfo.flags = 0;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &(this->vkState->descriptorSetLayout);
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = 0;

        VkResult result = vkCreatePipelineLayout(this->vkState->device, &pipelineLayoutCreateInfo, nullptr, &(this->vkState->pipelineLayout));
        CHECK_ERROR(result);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = nullptr;
        pipelineCreateInfo.flags = 0;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStageCreateInfos;
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pTessellationState = nullptr;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendingStateCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = this->vkState->pipelineLayout;
        pipelineCreateInfo.renderPass = this->vkState->renderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        result = vkCreateGraphicsPipelines(this->vkState->device, this->vkState->pipelineCache, 1, &pipelineCreateInfo, nullptr, &(this->vkState->pipeline));
        CHECK_ERROR(result);

        vkDestroyShaderModule(this->vkState->device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(this->vkState->device, vertexShaderModule, nullptr);
    }

    XR_API void Renderer::destroyGraphicsPipline()
    {
        vkDestroyPipeline(this->vkState->device, this->vkState->pipeline, nullptr);
        vkDestroyPipelineLayout(this->vkState->device, this->vkState->pipelineLayout, nullptr);
        this->vkState->pipeline = VK_NULL_HANDLE;
        this->vkState->pipelineLayout = VK_NULL_HANDLE;
    }

    VkFormat Renderer::findSupportedFormat(
        VkPhysicalDevice gpu,
        const std::vector<VkFormat> &formatsToCheck,
        VkImageTiling imageTiling,
        VkFormatFeatureFlags formatFeatureFlags
    )
    {
        for (VkFormat nextFormat : formatsToCheck)
        {
            VkFormatProperties formatProperties = {};
            vkGetPhysicalDeviceFormatProperties(gpu, nextFormat, &formatProperties);

            if (imageTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
            {
                return nextFormat;
            }

            if (imageTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
            {
                return nextFormat;
            }
        }

        return VK_FORMAT_UNDEFINED;
    }

    VkFormat Renderer::findDepthFormat()
    {
        std::vector<VkFormat> formatsToCheck = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

        return findSupportedFormat(this->vkState->gpuDetails.gpu, formatsToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool Renderer::hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || false;
    }

    XR_API void Renderer::initDepthStencilImage()
    {
        VkFormat depthStencilFormat = findDepthFormat();
        if (depthStencilFormat == VK_FORMAT_UNDEFINED)
        {
            assert(0 && "Depth stencil format not selected.");
        }

        createImage(
            this->vkState->surfaceSize.width,
            this->vkState->surfaceSize.height,
            1,
            this->vkState->msaaSamples,
            depthStencilFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->vkState->depthImage,
            this->vkState->depthImageMemory
        );

        createImageView(this->vkState->depthImage, depthStencilFormat, this->vkState->depthImageView, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    XR_API void Renderer::destoryDepthStencilImage()
    {
        vkDestroyImageView(this->vkState->device, this->vkState->depthImageView, nullptr);
        vkDestroyImage(this->vkState->device, this->vkState->depthImage, nullptr);
        vkFreeMemory(this->vkState->device, this->vkState->depthImageMemory, nullptr);
        this->vkState->depthImageView = VK_NULL_HANDLE;
        this->vkState->depthImage = VK_NULL_HANDLE;
        this->vkState->depthImageMemory = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initMSAAColorImage()
    {
        createImage(
            this->vkState->surfaceSize.width,
            this->vkState->surfaceSize.height,
            1,
            this->vkState->msaaSamples,
            this->vkState->surfaceFormat.format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->vkState->msaaColorImage,
            this->vkState->msaaColorImageMemory
        );

        createImageView(this->vkState->msaaColorImage, this->vkState->surfaceFormat.format, this->vkState->msaaColorImageView, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    XR_API void Renderer::destoryMSAAColorImage()
    {
        vkDestroyImageView(this->vkState->device, this->vkState->msaaColorImageView, nullptr);
        vkDestroyImage(this->vkState->device, this->vkState->msaaColorImage, nullptr);
        vkFreeMemory(this->vkState->device, this->vkState->msaaColorImageMemory, nullptr);
        this->vkState->msaaColorImageView = VK_NULL_HANDLE;
        this->vkState->msaaColorImage = VK_NULL_HANDLE;
        this->vkState->msaaColorImageMemory = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initRenderPass()
    {
        VkAttachmentDescription colorAttachmentDescription = {};
        colorAttachmentDescription.flags = 0;
        colorAttachmentDescription.format = this->vkState->surfaceFormat.format;
        colorAttachmentDescription.samples = this->vkState->msaaSamples;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthStencilAttachmentDescription = {};
        depthStencilAttachmentDescription.flags = 0;
        depthStencilAttachmentDescription.format = findDepthFormat();
        depthStencilAttachmentDescription.samples = this->vkState->msaaSamples;
        depthStencilAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthStencilAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthStencilAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthStencilAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthStencilAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthStencilAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolveDescription = {};
        colorAttachmentResolveDescription.flags = 0;
        colorAttachmentResolveDescription.format = this->vkState->surfaceFormat.format;
        colorAttachmentResolveDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolveDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolveDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolveDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolveDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolveDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolveDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentReference = {};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthStencilAttachmentReference = {};
        depthStencilAttachmentReference.attachment = 1;
        depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveReference = {};
        colorAttachmentResolveReference.attachment = 2;
        colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.flags = 0;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        subpassDescription.pResolveAttachments = &colorAttachmentResolveReference;
        subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachmentDescription,
                                                               depthStencilAttachmentDescription,
                                                               colorAttachmentResolveDescription };

        VkSubpassDependency subpassDependency = {};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext = nullptr;
        renderPassCreateInfo.flags = 0;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &subpassDependency;

        VkResult result = vkCreateRenderPass(this->vkState->device, &renderPassCreateInfo, nullptr, &(this->vkState->renderPass));
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destroyRenderPass()
    {
        vkDestroyRenderPass(this->vkState->device, this->vkState->renderPass, nullptr);
        this->vkState->renderPass = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboDescriptorSetLayoutBinding = {};
        uboDescriptorSetLayoutBinding.binding = 0;
        uboDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescriptorSetLayoutBinding.descriptorCount = 1;
        uboDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding = {};
        samplerDescriptorSetLayoutBinding.binding = 1;
        samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDescriptorSetLayoutBinding.descriptorCount = 1;
        samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = { uboDescriptorSetLayoutBinding, samplerDescriptorSetLayoutBinding };

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pNext = nullptr;
        descriptorSetLayoutCreateInfo.flags = 0;
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();

        VkResult result = vkCreateDescriptorSetLayout(this->vkState->device, &descriptorSetLayoutCreateInfo, nullptr, &this->vkState->descriptorSetLayout);
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destroyDescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(this->vkState->device, this->vkState->descriptorSetLayout, nullptr);
        this->vkState->descriptorSetLayout = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initFrameBuffers()
    {
        this->vkState->framebuffers.resize(this->vkState->swapchainImageCount);

        for (uint32_t swapchainImageCounter = 0; swapchainImageCounter < this->vkState->swapchainImageCount; ++swapchainImageCounter)
        {
            std::array<VkImageView, 3> attachments = { this->vkState->msaaColorImageView,
                                                       this->vkState->depthImageView,
                                                       this->vkState->swapchainImageViews[swapchainImageCounter] };

            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.pNext = nullptr;
            framebufferCreateInfo.flags = 0;
            framebufferCreateInfo.renderPass = this->vkState->renderPass;
            framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCreateInfo.pAttachments = attachments.data();
            framebufferCreateInfo.width = this->vkState->surfaceSize.width;
            framebufferCreateInfo.height = this->vkState->surfaceSize.height;
            framebufferCreateInfo.layers = 1;

            VkResult result =
                vkCreateFramebuffer(this->vkState->device, &framebufferCreateInfo, nullptr, &(this->vkState->framebuffers[swapchainImageCounter]));
            CHECK_ERROR(result);
        }
    }

    XR_API void Renderer::destroyFrameBuffers()
    {
        for (VkFramebuffer nextFrameBuffer : this->vkState->framebuffers)
        {
            vkDestroyFramebuffer(this->vkState->device, nextFrameBuffer, nullptr);
        }

        this->vkState->framebuffers.clear();
    }

    XR_API void Renderer::initCommandPool()
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = nullptr;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = this->vkState->queueFamilyIndices.graphicsFamilyIndex;

        VkResult result = vkCreateCommandPool(this->vkState->device, &commandPoolCreateInfo, nullptr, &(this->vkState->commandPool));
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destroyCommandPool()
    {
        vkDestroyCommandPool(this->vkState->device, this->vkState->commandPool, nullptr);
        this->vkState->commandPool = VK_NULL_HANDLE;
    }

    XR_API void Renderer::createImage(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkSampleCountFlagBits samplesCount,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkImage &image,
        VkDeviceMemory &imageMemory
    )
    {
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = samplesCount;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.usage = usage;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(this->vkState->device, &imageCreateInfo, nullptr, &image);
        CHECK_ERROR(result);

        VkMemoryRequirements imageMemoryRequirements = {};
        vkGetImageMemoryRequirements(this->vkState->device, image, &imageMemoryRequirements);

        uint32_t memoryIndex = findMemoryTypeIndex(&(this->vkState->gpuDetails.memoryProperties), &imageMemoryRequirements, memoryPropertyFlags);

        VkMemoryAllocateInfo memoryAllocationInfo = {};
        memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocationInfo.pNext = nullptr;
        memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
        memoryAllocationInfo.memoryTypeIndex = memoryIndex;

        result = vkAllocateMemory(this->vkState->device, &memoryAllocationInfo, nullptr, &imageMemory);
        CHECK_ERROR(result);

        vkBindImageMemory(this->vkState->device, image, imageMemory, 0);
    }

    XR_API void Renderer::createImageView(VkImage image, VkFormat format, VkImageView &imageView, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(this->vkState->device, &imageViewCreateInfo, nullptr, &imageView);
        CHECK_ERROR(result);
    }

    XR_API void Renderer::initTextureImage(const char *textureFilePath)
    {
        int textureWidth = 0;
        int textureHeight = 0;
        int textureChannels = 0;

        stbi_uc *pixels = stbi_load(textureFilePath, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            assert(0 && "Not able to load texture");
        }

        VkDeviceSize size = textureWidth * textureHeight * 4;
        this->vkState->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

        logf("---------- mipLevels: %d----------", this->vkState->mipLevels);

        VkBuffer stagingImageBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingImageBufferMemory = VK_NULL_HANDLE;

        createBuffer(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingImageBuffer,
            &stagingImageBufferMemory
        );

        void *data = nullptr;
        vkMapMemory(this->vkState->device, stagingImageBufferMemory, 0, size, 0, &data);
        memcpy(data, pixels, size);
        vkUnmapMemory(this->vkState->device, stagingImageBufferMemory);
        stbi_image_free(pixels);

        createImage(
            static_cast<uint32_t>(textureWidth),
            static_cast<uint32_t>(textureHeight),
            this->vkState->mipLevels,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->vkState->textureImage,
            this->vkState->textureImageMemory
        );

        transitionImageLayout(
            this->vkState->textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->vkState->mipLevels
        );

        copyBufferToImage(stagingImageBuffer, this->vkState->textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

        // Generate the mipmaps images and then transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
        generateMipmaps(this->vkState->textureImage, textureWidth, textureHeight, this->vkState->mipLevels);

        vkDestroyBuffer(this->vkState->device, stagingImageBuffer, nullptr);
        vkFreeMemory(this->vkState->device, stagingImageBufferMemory, nullptr);
    }

    XR_API void Renderer::destroyTextureImage()
    {
        vkDestroyImage(this->vkState->device, this->vkState->textureImage, nullptr);
        vkFreeMemory(this->vkState->device, this->vkState->textureImageMemory, nullptr);
        this->vkState->textureImage = VK_NULL_HANDLE;
        this->vkState->textureImageMemory = VK_NULL_HANDLE;
    }

    void Renderer::generateMipmaps(VkImage &image, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels)
    {
        int32_t mipWidth = textureWidth;
        int32_t mipHeight = textureHeight;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        beginOneTimeCommand(commandBuffer);

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = nullptr;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;
        imageMemoryBarrier.subresourceRange.levelCount = 1;

        // mip level starts from 1 not 0.
        for (uint32_t counter = 1; counter < mipLevels; ++counter)
        {
            // First, we transition level counter - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
            // This transition will wait for level counter - 1 to be filled, either from the previous blit command,
            // or from vkCmdCopyBufferToImage
            imageMemoryBarrier.subresourceRange.baseMipLevel = counter - 1;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

            vkCmdPipelineBarrier(
                commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
            );

            VkImageBlit imageBlit = {};

            // srcOffsets array determine the 3D region that data will be blitted from.
            imageBlit.srcOffsets[0] = { 0, 0, 0 };

            // The z dimension of srcOffsets[1] must be 1, since a 2D image has a depth of 1
            imageBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            // The source mip level is counter - 1
            imageBlit.srcSubresource.mipLevel = counter - 1;
            imageBlit.srcSubresource.baseArrayLayer = 0;
            imageBlit.srcSubresource.layerCount = 1;

            // dstOffsets determines the region that data will be blitted to.
            imageBlit.dstOffsets[0] = { 0, 0, 0 };

            // The x and y dimensions of the dstOffsets[1] are divided by two since
            // each mip level is half the size of the previous level.
            // The z dimension of dstOffsets[1] must be 1, since a 2D image has a depth of 1
            // In case where we have odd texture dimensions, the mip dimension may reach 1.
            // this will cause 0 to be passed to VkImageBlit.dstOffsets which results in validation layer warning.
            // To avoid this, check is next mip level is non-zero, if it is, then use 1 instead of 0.
            imageBlit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            // The destination mip level is counter
            imageBlit.dstSubresource.mipLevel = counter;
            imageBlit.dstSubresource.baseArrayLayer = 0;
            imageBlit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(
                commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR
            );

            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // This barrier transitions mip level i - 1 to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
            // This transition waits on the current blit command to finish.
            // All sampling operations will wait on this transition to finish.
            vkCmdPipelineBarrier(
                commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
            );

            // Divide the current mip dimensions by 2
            // Check each dimension before the division to ensure that dimension never becomes 0.
            // This handles cases where the image is not square, since one of the mip dimensions would reach 1 before the other dimension.
            // When this happens, that dimension should remain 1 for all remaining levels.
            if (mipWidth > 1)
            {
                mipWidth = mipWidth / 2;
            }

            if (mipHeight > 1)
            {
                mipHeight = mipHeight / 2;
            }
        }

        // Before we end the command buffer, we insert one more pipeline barrier.
        // This barrier transitions the last mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
        // This wasn't handled by the loop, since the last mip level is never blitted from.
        imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
        );

        // Now end the command buffer.
        endOneTimeCommand(commandBuffer);
    }

    XR_API void Renderer::initTextureImageView()
    {
        createImageView(
            this->vkState->textureImage, VK_FORMAT_R8G8B8A8_UNORM, this->vkState->textureImageView, VK_IMAGE_ASPECT_COLOR_BIT, this->vkState->mipLevels
        );
    }

    XR_API void Renderer::destroyTextureImageView()
    {
        vkDestroyImageView(this->vkState->device, this->vkState->textureImageView, nullptr);
        this->vkState->textureImageView = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initTextureSampler()
    {
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.pNext = nullptr;
        samplerCreateInfo.flags = 0;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = 16;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.minLod = 0;
        samplerCreateInfo.maxLod = static_cast<float>(this->vkState->mipLevels);
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        VkResult result = vkCreateSampler(this->vkState->device, &samplerCreateInfo, nullptr, &(this->vkState->textureSampler));
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destoryTextureSampler()
    {
        vkDestroySampler(this->vkState->device, this->vkState->textureSampler, nullptr);
        this->vkState->textureSampler = VK_NULL_HANDLE;
    }

    XR_API void Renderer::createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags bufferUsage,
        VkMemoryPropertyFlags memoryProperties,
        VkBuffer *buffer,
        VkDeviceMemory *bufferMemory
    )
    {
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.flags = 0;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = bufferUsage;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices = nullptr; // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT

        VkResult result = vkCreateBuffer(this->vkState->device, &bufferCreateInfo, nullptr, buffer);
        CHECK_ERROR(result);

        VkMemoryRequirements bufferMemoryRequirements = {};
        vkGetBufferMemoryRequirements(this->vkState->device, *buffer, &bufferMemoryRequirements);

        uint32_t memoryIndex = findMemoryTypeIndex(&(this->vkState->gpuDetails.memoryProperties), &bufferMemoryRequirements, memoryProperties);

        VkMemoryAllocateInfo memoryAllocationInfo = {};
        memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocationInfo.pNext = nullptr;
        memoryAllocationInfo.allocationSize = bufferMemoryRequirements.size;
        memoryAllocationInfo.memoryTypeIndex = memoryIndex;

        result = vkAllocateMemory(this->vkState->device, &memoryAllocationInfo, nullptr, bufferMemory);
        CHECK_ERROR(result);

        result = vkBindBufferMemory(this->vkState->device, *buffer, *bufferMemory, 0);
        CHECK_ERROR(result);
    }

    void Renderer::beginOneTimeCommand(VkCommandBuffer &commandBuffer)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = nullptr;
        commandBufferAllocateInfo.commandPool = this->vkState->commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VkResult result = vkAllocateCommandBuffers(this->vkState->device, &commandBufferAllocateInfo, &commandBuffer);
        CHECK_ERROR(result);

        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    }

    void Renderer::endOneTimeCommand(VkCommandBuffer &commandBuffer)
    {
        VkResult result = vkEndCommandBuffer(commandBuffer);
        CHECK_ERROR(result);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->vkState->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        CHECK_ERROR(result);

        vkQueueWaitIdle(this->vkState->graphicsQueue);
        vkFreeCommandBuffers(this->vkState->device, this->vkState->commandPool, 1, &commandBuffer);
    }

    void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, uint32_t mipLevels)
    {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        beginOneTimeCommand(commandBuffer);

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = nullptr;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStageMask;
        VkPipelineStageFlags destinationStageMask;

        if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            assert(0 && "Unsupported layout transition!!!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStageMask, destinationStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        endOneTimeCommand(commandBuffer);
    }

    XR_API void Renderer::copyBuffer(VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        beginOneTimeCommand(commandBuffer);

        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, sourceBuffer, targetBuffer, 1, &copyRegion);

        endOneTimeCommand(commandBuffer);
    }

    XR_API void Renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        beginOneTimeCommand(commandBuffer);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset.x = 0;
        region.imageOffset.y = 0;
        region.imageOffset.z = 0;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endOneTimeCommand(commandBuffer);
    }

    XR_API void Renderer::loadModel(const char *modelFilePath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string error;

        bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &error, modelFilePath);

        if (!loaded)
        {
            logf("Model load error: %s", error.c_str());
            assert(0 && "Not able to load model.");
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        for (const tinyobj::shape_t &nextShape : shapes)
        {
            for (const tinyobj::index_t &nextIndex : nextShape.mesh.indices)
            {
                Vertex nextVertex = {};
                nextVertex.position = { // the attrib.vertices array is an array of float values instead of something like glm::vec3,
                                        // so you need to multiply the index by 3 to create group of 3 values.
                                        attrib.vertices[3 * nextIndex.vertex_index + 0],
                                        attrib.vertices[3 * nextIndex.vertex_index + 1],
                                        attrib.vertices[3 * nextIndex.vertex_index + 2]
                };

                nextVertex.textureCoordinates = { // the attrib.texcoords array is an array of float values instead of something like glm::vec2,
                                                  // so you need to multiply the index by 2 to create group of 2 values.
                                                  attrib.texcoords[2 * nextIndex.texcoord_index + 0],
                                                  1.0 - attrib.texcoords[2 * nextIndex.texcoord_index + 1]
                };

                nextVertex.color = { 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(nextVertex) == 0)
                {
                    uniqueVertices[nextVertex] = static_cast<uint32_t>(this->vkState->vertices.size());
                    this->vkState->vertices.push_back(nextVertex);
                }

                this->vkState->vertexIndices.push_back(uniqueVertices[nextVertex]);
            }
        }
    }

    XR_API void Renderer::initVertexBuffer()
    {
        VkDeviceSize size = sizeof(this->vkState->vertices[0]) * this->vkState->vertices.size();
        VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

        createBuffer(size, stagingBufferUsage, stagingMemoryProperties, &stagingBuffer, &stagingBufferMemory);

        void *stagingBufferData = nullptr;
        VkResult result = vkMapMemory(this->vkState->device, stagingBufferMemory, 0, size, 0, &stagingBufferData);
        CHECK_ERROR(result);

        memcpy(stagingBufferData, this->vkState->vertices.data(), (size_t)size);
        vkUnmapMemory(this->vkState->device, stagingBufferMemory);

        VkBufferUsageFlags vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkMemoryPropertyFlags vertexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        createBuffer(size, vertexBufferUsage, vertexMemoryProperties, &(this->vkState->vertexBuffer), &(this->vkState->vertexBufferMemory));
        copyBuffer(stagingBuffer, this->vkState->vertexBuffer, size);
        vkDestroyBuffer(this->vkState->device, stagingBuffer, nullptr);
        vkFreeMemory(this->vkState->device, stagingBufferMemory, nullptr);
    }

    XR_API void Renderer::destroyVertexBuffer()
    {
        vkDestroyBuffer(this->vkState->device, this->vkState->vertexBuffer, nullptr);
        vkFreeMemory(this->vkState->device, this->vkState->vertexBufferMemory, nullptr);
        this->vkState->vertexBuffer = VK_NULL_HANDLE;
        this->vkState->vertexBufferMemory = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initIndexBuffer()
    {
        VkDeviceSize size = sizeof(this->vkState->vertexIndices[0]) * this->vkState->vertexIndices.size();
        VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

        createBuffer(size, stagingBufferUsage, stagingMemoryProperties, &stagingBuffer, &stagingBufferMemory);

        void *stagingBufferData = nullptr;
        VkResult result = vkMapMemory(this->vkState->device, stagingBufferMemory, 0, size, 0, &stagingBufferData);
        CHECK_ERROR(result);

        memcpy(stagingBufferData, this->vkState->vertexIndices.data(), (size_t)size);
        vkUnmapMemory(this->vkState->device, stagingBufferMemory);

        VkBufferUsageFlags indexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VkMemoryPropertyFlags indexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        createBuffer(size, indexBufferUsage, indexMemoryProperties, &(this->vkState->indexBuffer), &(this->vkState->indexBufferMemory));
        copyBuffer(stagingBuffer, this->vkState->indexBuffer, size);
        vkDestroyBuffer(this->vkState->device, stagingBuffer, nullptr);
        vkFreeMemory(this->vkState->device, stagingBufferMemory, nullptr);
    }

    XR_API void Renderer::destroyIndexBuffer()
    {
        vkDestroyBuffer(this->vkState->device, this->vkState->indexBuffer, nullptr);
        vkFreeMemory(this->vkState->device, this->vkState->indexBufferMemory, nullptr);
        this->vkState->indexBuffer = VK_NULL_HANDLE;
        this->vkState->indexBufferMemory = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initUniformBuffers()
    {
        VkDeviceSize size = sizeof(xr::UniformBufferObject);
        VkBufferUsageFlags uniformBufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkMemoryPropertyFlags uniformMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        this->vkState->uniformBuffers.resize(this->vkState->swapchainImages.size());
        this->vkState->uniformBuffersMemory.resize(this->vkState->swapchainImages.size());

        for (size_t counter = 0; counter < this->vkState->swapchainImages.size(); ++counter)
        {
            createBuffer(
                size, uniformBufferUsage, uniformMemoryProperties, &(this->vkState->uniformBuffers[counter]), &(this->vkState->uniformBuffersMemory[counter])
            );
        }
    }

    XR_API void Renderer::destroyUniformBuffers()
    {
        for (size_t counter = 0; counter < this->vkState->swapchainImages.size(); ++counter)
        {
            vkDestroyBuffer(this->vkState->device, this->vkState->uniformBuffers[counter], nullptr);
            vkFreeMemory(this->vkState->device, this->vkState->uniformBuffersMemory[counter], nullptr);
        }

        this->vkState->swapchainImages.clear();
        this->vkState->uniformBuffers.clear();
        this->vkState->uniformBuffersMemory.clear();
    }

    XR_API void Renderer::initDescriptorPool()
    {
        VkDescriptorPoolSize uboPoolSize = {};
        uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboPoolSize.descriptorCount = static_cast<uint32_t>(this->vkState->swapchainImages.size());

        VkDescriptorPoolSize samplerPoolSize = {};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerPoolSize.descriptorCount = static_cast<uint32_t>(this->vkState->swapchainImages.size());
        ;

        std::array<VkDescriptorPoolSize, 2> poolSizes = { uboPoolSize, samplerPoolSize };

        VkDescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.pNext = nullptr;
        poolCreateInfo.flags = 0;
        // If you want to explicitly destroy the descriptorSet, then set this bit
        // else you will get runtime error while destroying the descriptorSet.
        // We are not going to used this for now.
        // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolCreateInfo.maxSets = static_cast<uint32_t>(this->vkState->swapchainImages.size());
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolCreateInfo.pPoolSizes = poolSizes.data();

        VkResult result = vkCreateDescriptorPool(this->vkState->device, &poolCreateInfo, nullptr, &(this->vkState->descriptorPool));
        CHECK_ERROR(result);
    }

    XR_API void Renderer::destroyDescriptorPool()
    {
        vkDestroyDescriptorPool(this->vkState->device, this->vkState->descriptorPool, nullptr);
        this->vkState->descriptorPool = VK_NULL_HANDLE;
    }

    XR_API void Renderer::initDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(this->vkState->swapchainImages.size(), this->vkState->descriptorSetLayout);

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = nullptr;
        descriptorSetAllocateInfo.descriptorPool = this->vkState->descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(this->vkState->swapchainImages.size());
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        this->vkState->descriptorSets.resize(this->vkState->swapchainImages.size());

        VkResult result = vkAllocateDescriptorSets(this->vkState->device, &descriptorSetAllocateInfo, this->vkState->descriptorSets.data());
        CHECK_ERROR(result);

        for (size_t counter = 0; counter < this->vkState->swapchainImages.size(); ++counter)
        {
            VkDescriptorBufferInfo descriptorBufferInfo = {};
            descriptorBufferInfo.buffer = this->vkState->uniformBuffers[counter];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(xr::UniformBufferObject);

            VkDescriptorImageInfo descriptorImageInfo = {};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorImageInfo.imageView = this->vkState->textureImageView;
            descriptorImageInfo.sampler = this->vkState->textureSampler;

            VkWriteDescriptorSet uniformBudderDescriptorWrite = {};
            uniformBudderDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uniformBudderDescriptorWrite.pNext = nullptr;
            uniformBudderDescriptorWrite.dstSet = this->vkState->descriptorSets[counter];
            uniformBudderDescriptorWrite.dstBinding = 0;
            uniformBudderDescriptorWrite.dstArrayElement = 0;
            uniformBudderDescriptorWrite.descriptorCount = 1;
            uniformBudderDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformBudderDescriptorWrite.pImageInfo = nullptr;
            uniformBudderDescriptorWrite.pBufferInfo = &descriptorBufferInfo;
            uniformBudderDescriptorWrite.pTexelBufferView = nullptr;

            VkWriteDescriptorSet textureImageDescriptorWrite = {};
            textureImageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            textureImageDescriptorWrite.pNext = nullptr;
            textureImageDescriptorWrite.dstSet = this->vkState->descriptorSets[counter];
            textureImageDescriptorWrite.dstBinding = 1;
            textureImageDescriptorWrite.dstArrayElement = 0;
            textureImageDescriptorWrite.descriptorCount = 1;
            textureImageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureImageDescriptorWrite.pImageInfo = &descriptorImageInfo;
            textureImageDescriptorWrite.pBufferInfo = nullptr;
            textureImageDescriptorWrite.pTexelBufferView = nullptr;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = { uniformBudderDescriptorWrite, textureImageDescriptorWrite };
            vkUpdateDescriptorSets(this->vkState->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    XR_API void Renderer::destroyDescriptorSets()
    {
        // If you want to explicitly destroy the descriptorSet, then set
        // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        // bit in VkDescriptorPoolCreateInfo else you will get runtime error
        // while destroying the descriptorSet.
        // We are not going to used this for now.
        // vkFreeDescriptorSets(
        //     this->vkState->device,
        //     this->vkState->descriptorPool,
        //     this->vkState->descriptorSets.size(),
        //     this->vkState->descriptorSets.data()
        // );

        this->vkState->descriptorSets.clear();
    }

    XR_API void Renderer::initCommandBuffers()
    {
        this->vkState->commandBuffers.resize(this->vkState->framebuffers.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = nullptr;
        commandBufferAllocateInfo.commandPool = this->vkState->commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(this->vkState->commandBuffers.size());

        VkResult result = vkAllocateCommandBuffers(this->vkState->device, &commandBufferAllocateInfo, this->vkState->commandBuffers.data());
        CHECK_ERROR(result);

        for (uint32_t counter = 0; counter < this->vkState->commandBuffers.size(); ++counter)
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo = {};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.pNext = nullptr;
            commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            commandBufferBeginInfo.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(this->vkState->commandBuffers[counter], &commandBufferBeginInfo);

            VkRect2D renderArea = {};
            renderArea.offset.x = 0;
            renderArea.offset.y = 0;
            renderArea.extent.width = this->vkState->surfaceSize.width;
            renderArea.extent.height = this->vkState->surfaceSize.height;

            std::array<VkClearValue, 2> clearValue = {};
            clearValue[0].color = { 0.0f, 0.0f, 0.0f, 1.0f }; // {r, g, b, a}
            clearValue[1].depthStencil = { 1.0f, 0 };         // {depth, stencil}

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.pNext = nullptr;
            renderPassBeginInfo.renderPass = this->vkState->renderPass;
            renderPassBeginInfo.framebuffer = this->vkState->framebuffers[counter];
            renderPassBeginInfo.renderArea = renderArea;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
            renderPassBeginInfo.pClearValues = clearValue.data();

            vkCmdBeginRenderPass(this->vkState->commandBuffers[counter], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(this->vkState->commandBuffers[counter], VK_PIPELINE_BIND_POINT_GRAPHICS, this->vkState->pipeline);

            std::vector<VkBuffer> vertexBuffers = { this->vkState->vertexBuffer };
            std::vector<VkDeviceSize> offsets = { 0 };
            vkCmdBindVertexBuffers(this->vkState->commandBuffers[counter], 0, 1, vertexBuffers.data(), offsets.data());
            vkCmdBindIndexBuffer(this->vkState->commandBuffers[counter], this->vkState->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(
                this->vkState->commandBuffers[counter],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                this->vkState->pipelineLayout,
                0,
                1,
                &(this->vkState->descriptorSets[counter]),
                0,
                nullptr
            );

            vkCmdDrawIndexed(this->vkState->commandBuffers[counter], static_cast<uint32_t>(this->vkState->vertexIndices.size()), 1, 0, 0, 0);
            vkCmdEndRenderPass(this->vkState->commandBuffers[counter]);

            VkResult result = vkEndCommandBuffer(this->vkState->commandBuffers[counter]);
            CHECK_ERROR(result);
        }
    }

    XR_API void Renderer::destroyCommandBuffers()
    {
        vkFreeCommandBuffers(
            this->vkState->device, this->vkState->commandPool, static_cast<uint32_t>(this->vkState->commandBuffers.size()), this->vkState->commandBuffers.data()
        );

        this->vkState->commandBuffers.clear();
    }

    XR_API void Renderer::initSynchronizations()
    {
        this->vkState->imageAvailableSemaphores.resize(this->vkState->MAX_FRAMES_IN_FLIGHT);
        this->vkState->renderFinishedSemaphores.resize(this->vkState->MAX_FRAMES_IN_FLIGHT);
        this->vkState->inFlightFences.resize(this->vkState->MAX_FRAMES_IN_FLIGHT);
        this->vkState->inFlightImages.resize(this->vkState->swapchainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = nullptr;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t counter = 0; counter < this->vkState->MAX_FRAMES_IN_FLIGHT; ++counter)
        {
            VkResult result = vkCreateSemaphore(this->vkState->device, &semaphoreCreateInfo, nullptr, &this->vkState->imageAvailableSemaphores[counter]);
            CHECK_ERROR(result);

            result = vkCreateSemaphore(this->vkState->device, &semaphoreCreateInfo, nullptr, &this->vkState->renderFinishedSemaphores[counter]);
            CHECK_ERROR(result);

            result = vkCreateFence(this->vkState->device, &fenceCreateInfo, nullptr, &(this->vkState->inFlightFences[counter]));
            CHECK_ERROR(result);
        }
    }

    XR_API void Renderer::destroySynchronizations()
    {
        for (size_t counter = 0; counter < this->vkState->MAX_FRAMES_IN_FLIGHT; ++counter)
        {
            vkDestroySemaphore(this->vkState->device, this->vkState->imageAvailableSemaphores[counter], nullptr);
            vkDestroySemaphore(this->vkState->device, this->vkState->renderFinishedSemaphores[counter], nullptr);
            vkDestroyFence(this->vkState->device, this->vkState->inFlightFences[counter], nullptr);
        }

        this->vkState->imageAvailableSemaphores.clear();
        this->vkState->renderFinishedSemaphores.clear();
        this->vkState->inFlightFences.clear();
    }

    XR_API void Renderer::recreateSwapChain()
    {
        logf("---------- Recreate SwapChain --------");
        vkDeviceWaitIdle(this->vkState->device);
        cleanupSwapChain();
        initSwapchain();
        initSwapchainImageViews();
        initRenderPass();
        initGraphicsPiplineCache();
        initGraphicsPipline();
        initDepthStencilImage();
        initMSAAColorImage();
        initFrameBuffers();
        initUniformBuffers();
        initDescriptorPool();
        initDescriptorSets();
        initCommandBuffers();
    }

    XR_API void Renderer::cleanupSwapChain()
    {
        waitForIdle();
        destroyCommandBuffers();
        destroyDescriptorSets();
        destroyDescriptorPool();
        destroyUniformBuffers();
        destroyFrameBuffers();
        destoryMSAAColorImage();
        destoryDepthStencilImage();
        destroyGraphicsPipline();
        destroyGraphicsPiplineCache();
        destroyRenderPass();
        destroySwapchainImageViews();
        destroySwapchain();
    }

    XR_API void Renderer::render(xr::UniformBufferObject *ubo)
    {
        vkWaitForFences(this->vkState->device, 1, &(this->vkState->inFlightFences[this->vkState->currentFrame]), VK_TRUE, UINT64_MAX);

        uint32_t activeSwapchainImageId = UINT32_MAX;

        VkResult result = vkAcquireNextImageKHR(
            this->vkState->device,
            this->vkState->swapchain,
            UINT64_MAX,
            this->vkState->imageAvailableSemaphores[this->vkState->currentFrame],
            VK_NULL_HANDLE,
            &activeSwapchainImageId
        );

        // If result is VK_ERROR_OUT_OF_DATE_KHR than just recreate swap chain
        // as current swap chain cannot be used with current surface.
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }

        CHECK_ERROR(result);

        // Update the uniform buffer for current image.
        updateUniformBuffer(activeSwapchainImageId, ubo);

        if (this->vkState->inFlightImages[activeSwapchainImageId] != VK_NULL_HANDLE)
        {
            vkWaitForFences(this->vkState->device, 1, &this->vkState->inFlightImages[activeSwapchainImageId], VK_TRUE, UINT64_MAX);
        }

        this->vkState->inFlightImages[activeSwapchainImageId] = this->vkState->inFlightFences[this->vkState->currentFrame];

        VkSemaphore waitSemaphores[] = { this->vkState->imageAvailableSemaphores[this->vkState->currentFrame] };
        VkSemaphore signalSemaphores[] = { this->vkState->renderFinishedSemaphores[this->vkState->currentFrame] };
        VkPipelineStageFlags waitPipelineStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(sizeof(waitSemaphores) / sizeof(waitSemaphores[0]));
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitPipelineStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &(this->vkState->commandBuffers[activeSwapchainImageId]);
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(sizeof(signalSemaphores) / sizeof(signalSemaphores[0]));
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(this->vkState->device, 1, &(this->vkState->inFlightFences[this->vkState->currentFrame]));

        result = vkQueueSubmit(this->vkState->graphicsQueue, 1, &submitInfo, this->vkState->inFlightFences[this->vkState->currentFrame]);
        CHECK_ERROR(result);

        VkSwapchainKHR swapchains[] = { this->vkState->swapchain };

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.waitSemaphoreCount = static_cast<uint32_t>(sizeof(signalSemaphores) / sizeof(signalSemaphores[0]));
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = static_cast<uint32_t>(sizeof(swapchains) / sizeof(swapchains[0]));
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &activeSwapchainImageId;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(this->vkState->presentQueue, &presentInfo);

        // Recreate the swap chain if result is suboptimal,
        // because we want the best possible result.
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            recreateSwapChain();
        }
        else
        {
            CHECK_ERROR(result);
        }

        this->vkState->currentFrame = (this->vkState->currentFrame + 1) % this->vkState->MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::updateUniformBuffer(uint32_t imageIndex, xr::UniformBufferObject *ubo)
    {
        void *data = nullptr;
        vkMapMemory(this->vkState->device, this->vkState->uniformBuffersMemory[imageIndex], 0, sizeof(xr::UniformBufferObject), 0, &data);
        memcpy(data, ubo, sizeof(xr::UniformBufferObject));
        vkUnmapMemory(this->vkState->device, this->vkState->uniformBuffersMemory[imageIndex]);
    }

    // Debug methods

    void Renderer::printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount)
    {
        if (!properties)
        {
            logf("No GPU properties to show!!!");
            return;
        }

        logf("---------- GPU Properties [%d/%d]----------", currentGpuIndex, totalGpuCount);
        logf("Device Name\t\t: %s", properties->deviceName);
        logf("Vendor Id\t\t: %d", properties->vendorID);
        logf("Device Id\t\t: %d", properties->deviceID);
        logf("Device Type\t\t: %d", properties->deviceType);
        logf("API Version\t\t: %d", properties->apiVersion);
        logf("Driver Version\t\t: %d", properties->driverVersion);
        log_uuid("Pipeline Cache UUID\t: ", properties->pipelineCacheUUID);
        logf("---------- GPU Properties End ----------");
    }

    void Renderer::printInstanceLayerProperties(std::vector<VkLayerProperties> properties)
    {
#ifndef NDEBUG

        logf("---------- Instance Layer Properties ----------");

        for (VkLayerProperties &nextProperty : properties)
        {
            logf("Layer Name\t\t: %s", nextProperty.layerName);
            logf("Description\t\t: %s", nextProperty.description);
            logf("Spec Version\t\t: %d", nextProperty.specVersion);
            logf("Implementation Version\t: %d", nextProperty.implementationVersion);
            logf("------------------------------------------------------------");
        }

        logf("---------- Instance Layer Properties End [%d] ----------", properties.size());

#endif
    }

    void Renderer::printDeviceLayerProperties(std::vector<VkLayerProperties> properties)
    {
#ifndef NDEBUG

        logf("---------- Device Layer Properties ----------");

        for (VkLayerProperties &nextProperty : properties)
        {
            logf("Layer Name\t\t: %s", nextProperty.layerName);
            logf("Description\t\t: %s", nextProperty.description);
            logf("Spec Version\t\t: %d", nextProperty.specVersion);
            logf("Implementation Version\t: %d", nextProperty.implementationVersion);
            logf("------------------------------------------------------------");
        }

        logf("---------- Device Layer Properties End [%d] ----------", properties.size());

#endif
    }

    void Renderer::printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats)
    {
#ifndef NDEBUG

        logf("---------- Surface Formats ----------");

        for (VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
        {
            logf("format\t\t: %d", nextSurfaceFormat.format);
            logf("colorSpace\t: %d", nextSurfaceFormat.colorSpace);
            logf("------------------------------------------------------------");
        }

        logf("---------- Surface Formats Details End [%d] ----------", surfaceFormats.size());

#endif
    }

    void Renderer::printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount)
    {
#ifndef NDEBUG

        logf("---------- Swapchain Image Count ----------");
        logf("Min\t: %d", minImageCount);
        logf("Max\t: %d", maxImageCount);
        logf("Current\t: %d", currentImageCount);
        logf("---------- Swapchain Image Count End ----------");

#endif
    }
} // namespace xr
