#include <xRenderer/platform.h>

#if defined(VK_USE_PLATFORM_XCB_KHR)

#include <xRenderer/logger.h>
#include <xRenderer/debugger.h>
#include <xRenderer/model.h>

#include "lib/stb/stb_image.h"

#include "vulkanWindow.h"
#include "resource.h"
#include "utils.h"

xcb_connection_t *xcbConnection = NULL;
xcb_screen_t *xcbScreen = NULL;
xcb_window_t xcbWindow;
xcb_intern_atom_reply_t *atom_wm_delete_window_reply = NULL;

bool isCloseButtonClicked = false;

int main(void);
void handleEvent(const xcb_generic_event_t *event);

XrModel *homeModel = VK_NULL_HANDLE;
void updateHomeModel(XrUniformBuffer *);

XrModel *vikingRoomModel = VK_NULL_HANDLE;
void updateVikingRoomModel(XrUniformBuffer *);

uint32_t modelsCount = 2;
XrModel **models = VK_NULL_HANDLE;

void handleEvent(const xcb_generic_event_t *event)
{
    switch (event->response_type & 0x7f)
    {
        case XCB_CLIENT_MESSAGE:
        {
            const xcb_client_message_event_t *clientMessageEvent = (xcb_client_message_event_t *)event;
            if (clientMessageEvent->data.data32[0] == atom_wm_delete_window_reply->atom)
            {
                isCloseButtonClicked = true;
            }
        }
        break;

        case XCB_KEY_PRESS:
        {
            const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
            switch (keyEvent->detail)
            {
                case 0x9: // Escape key code.
                    isEscapeKeyPressed = true;
                    break;

                case 0x29: // 'f; key code
                    isFullscreen = !isFullscreen;
                    toggleFullscreen(isFullscreen);
                    break;

                default:
                    break;
            }
        }
        break;

        case XCB_DESTROY_NOTIFY:
            isCloseButtonClicked = true;
            break;

        case XCB_CONFIGURE_NOTIFY:
        {
            const xcb_configure_notify_event_t *configureEvent = (const xcb_configure_notify_event_t *)event;
            resize((uint32_t)(configureEvent->width), (uint32_t)(configureEvent->height));
        }
        break;

        default:
            break;
    }
}

int main(void)
{
    VkResult vkResult = VK_SUCCESS;

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | XWindows";

    context = (XrContext *)malloc(sizeof(XrContext));
    memset((void *)context, 0, sizeof(XrContext));
    context->surfaceExtent.width = 800;
    context->surfaceExtent.height = 600;

    xrCreateLogger("debug_win32.log", &(context->logger));

    initializePlatformSpecificWindow(context);

    vkResult = initializeVulkan(context);

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to initialize Vulkan (%d)", vkResult);
        cleanUp(context);
        exit(EXIT_FAILURE);
    }

    int returnCode = mainLoop(context);

    cleanUp(context);

    return returnCode;
}

void initializePlatformSpecificWindow(XrContext *context)
{
    uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t valueList[32] = {0};

    const xcb_setup_t *setup = NULL;
    xcb_screen_iterator_t screenIterator;
    xcb_generic_error_t *error = NULL;
    xcb_void_cookie_t cookieForError;
    int screenCount = 0;

    xcbConnection = xcb_connect(NULL, &screenCount);

    if (xcbConnection == NULL)
    {
        XR_LOG_ERROR(context->logger, "Error: Unable to open XgDisplay.\n");
        cleanUp(context);
        std::exit(EXIT_FAILURE);
    }

    setup = xcb_get_setup(xcbConnection);
    screenIterator = xcb_setup_roots_iterator(setup);

    while (screenCount > 0)
    {
        --screenCount;
        xcb_screen_next(&screenIterator);
    }

    xcbScreen = screenIterator.data;
    xcbWindow = xcb_generate_id(xcbConnection);

    valueList[0] = xcbScreen->black_pixel;
    valueList[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                   XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE;

    cookieForError = xcb_create_window(
        xcbConnection,
        XCB_COPY_FROM_PARENT,
        xcbWindow,
        xcbScreen->root,
        0,
        0,
        context->surfaceExtent.width,
        context->surfaceExtent.height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        xcbScreen->root_visual,
        valueMask,
        valueList
    );

    error = xcb_request_check(xcbConnection, cookieForError);

    if (error)
    {
        XR_LOG_INFO(context->logger, "Error: Cannot create window [%d]", error->error_code);
        free(error);
        cleanUp(context);
        exit(EXIT_FAILURE);
    }

    // Enable the window close button action.
    xcb_intern_atom_cookie_t atom_wm_protocols_cookie = xcb_intern_atom(xcbConnection, 1, std::strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *atom_wm_protocols_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_protocols_cookie, NULL);

    xcb_intern_atom_cookie_t atom_wm_delete_window_cookie = xcb_intern_atom(xcbConnection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    atom_wm_delete_window_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_delete_window_cookie, NULL);

    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, xcbWindow, atom_wm_protocols_reply->atom, 4, 32, 1, &(atom_wm_delete_window_reply->atom));

    // Update the window title.
    xcb_change_property(
        xcbConnection,
        XCB_PROP_MODE_REPLACE,
        xcbWindow,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        std::string(windowTitle.begin(), windowTitle.end()).size(),
        std::string(windowTitle.begin(), windowTitle.end()).c_str()
    );

    free(atom_wm_protocols_reply);
    xcb_map_window(xcbConnection, xcbWindow);
    xcb_flush(xcbConnection);
}

void destroyPlatformSpecificWindow()
{
    if (xcbWindow)
    {
        xcb_destroy_window(xcbConnection, xcbWindow);
    }

    if (xcbConnection)
    {
        xcb_disconnect(xcbConnection);
        xcbConnection = VK_NULL_HANDLE;
    }
}

void rankDevice(XrContext *context, XrPhysicalDevice *gpuDetails)
{
    // Higher the rank, more suitable the device
    uint32_t rank = 0;
    bool extensionSupported = checkDeviceExtensionSupport(context, gpuDetails->gpu);
    bool swapchainSupported = false;

    if (extensionSupported)
    {
        XrSwapchainSupport details = {};
        xrQuerySwapchainSupportDetails(context, gpuDetails->gpu, &details);
        swapchainSupported = !details.surfaceFormatsCount && !details.presentModesCount;
    }

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(gpuDetails->gpu, &supportedFeatures);

    // Increase the rank for each check
    if (gpuDetails->graphicsFamilyIndex != UINT32_MAX)
    {
        rank++;
    }

    if (gpuDetails->hasSeparatePresentQueue && gpuDetails->presentFamilyIndex != UINT32_MAX)
    {
        rank++;
    }

    if (extensionSupported)
    {
        rank++;
    }

    if (swapchainSupported)
    {
        rank++;
    }

    if (supportedFeatures.samplerAnisotropy)
    {
        rank++;
    }

    if (supportedFeatures.geometryShader)
    {
        rank++;
    }

    if (supportedFeatures.tessellationShader)
    {
        rank++;
    }

    if (gpuDetails->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        rank += 2;
    }
    else if (gpuDetails->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        rank++;
    }

    gpuDetails->rank = rank;
}

void findSuitableDeviceQueues(XrContext *context, XrPhysicalDevice *gpuDetails)
{
    uint32_t familyCount = 0;
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(gpuDetails->gpu, &familyCount, VK_NULL_HANDLE);
    VkQueueFamilyProperties *familyPropertiesList = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * familyCount);
    memset((void *)familyPropertiesList, 0, sizeof(VkQueueFamilyProperties) * familyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(gpuDetails->gpu, &familyCount, familyPropertiesList);

    for (uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
    {
        VkQueueFamilyProperties *nextFamilyProperties = &familyPropertiesList[queueCounter];

        if (nextFamilyProperties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamilyIndex = queueCounter;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpuDetails->gpu, queueCounter, context->surface, &presentSupport);

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
            vkGetPhysicalDeviceSurfaceSupportKHR(gpuDetails->gpu, queueCounter, context->surface, &presentSupport);

            if (presentSupport == VK_TRUE)
            {
                presentFamilyIndex = queueCounter;
                break;
            }
        }
    }

    gpuDetails->graphicsFamilyIndex = graphicsFamilyIndex;
    gpuDetails->presentFamilyIndex = presentFamilyIndex;
    gpuDetails->hasSeparatePresentQueue = (presentFamilyIndex != graphicsFamilyIndex);

    XR_FREE(familyPropertiesList);
}

void findMaxMSAASampleCount(XrPhysicalDevice *gpuDetails)
{
    VkSampleCountFlags sampleCountFlags =
        gpuDetails->properties.limits.framebufferColorSampleCounts & gpuDetails->properties.limits.framebufferDepthSampleCounts;

    if (sampleCountFlags & VK_SAMPLE_COUNT_64_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_64_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_32_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_32_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_16_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_16_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_8_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_8_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_4_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_4_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_2_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_2_BIT;
    }
    else
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    }
}

bool checkDeviceExtensionSupport(XrContext *context, VkPhysicalDevice gpu)
{
    uint32_t availableDeviceExtensionsCount = 0;

    VkResult vkResult = vkEnumerateDeviceExtensionProperties(gpu, VK_NULL_HANDLE, &availableDeviceExtensionsCount, VK_NULL_HANDLE);

    if (XR_IS_ERROR(vkResult))
    {
        return false;
    }

    if (availableDeviceExtensionsCount == 0)
    {
        return false;
    }

    VkExtensionProperties *availableDeviceExtensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * availableDeviceExtensionsCount);
    memset((void *)availableDeviceExtensions, 0, sizeof(VkExtensionProperties) * availableDeviceExtensionsCount);

    vkResult = vkEnumerateDeviceExtensionProperties(gpu, VK_NULL_HANDLE, &availableDeviceExtensionsCount, availableDeviceExtensions);

    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(availableDeviceExtensions);
        return false;
    }

    VkBool32 found = VK_FALSE;

    for (uint32_t requiredExtensionsIndex = 0; requiredExtensionsIndex < context->deviceExtensionsCount; ++requiredExtensionsIndex)
    {
        found = VK_FALSE;

        for (uint32_t availableExtensionsIndex = 0; availableExtensionsIndex < availableDeviceExtensionsCount; ++availableExtensionsIndex)
        {
            if (strcmp(context->deviceExtensions[requiredExtensionsIndex], availableDeviceExtensions[availableExtensionsIndex].extensionName) == 0)
            {
                found = VK_TRUE;
                break;
            }
        }

        if (found != VK_TRUE)
        {
            break;
        }
    }

    XR_FREE(availableDeviceExtensions);

    return found == VK_TRUE;
}

void printGpuProperties(XrContext *context, XrPhysicalDevice *gpu, uint32_t currentGpuIndex, uint32_t totalGpuCount)
{
    if (!gpu)
    {
        XR_LOG_INFO(context->logger, "No GPU properties to show!!!");
        return;
    }

    XR_LOG_INFO(context->logger, "---------- GPU Properties [%d/%d] [Rank: %d] ----------", currentGpuIndex, totalGpuCount, gpu->rank);
    XR_LOG_INFO(context->logger, "Device Name               : %s", gpu->properties.deviceName);
    XR_LOG_INFO(context->logger, "Vendor Id                 : %d", gpu->properties.vendorID);
    XR_LOG_INFO(context->logger, "Device Id                 : %d", gpu->properties.deviceID);
    XR_LOG_INFO(context->logger, "Device Type               : %d", gpu->properties.deviceType);
    XR_LOG_INFO(context->logger, "API Version               : %d", gpu->properties.apiVersion);
    XR_LOG_INFO(context->logger, "Driver Version            : %d", gpu->properties.driverVersion);
    XR_LOG_UUID(context->logger, "Pipeline Cache UUID       : ", gpu->properties.pipelineCacheUUID);
    XR_LOG_INFO(context->logger, "Graphics Family Index     : %d", gpu->graphicsFamilyIndex);
    XR_LOG_INFO(context->logger, "Present Family Index      : %d", gpu->presentFamilyIndex);
    XR_LOG_INFO(context->logger, "Has Separate Present Queue: %d", gpu->hasSeparatePresentQueue);
    XR_LOG_INFO(context->logger, "MSAA samples count        : %d", gpu->msaaSamples);
    XR_LOG_INFO(context->logger, "---------- GPU Properties End ----------");
}

VkResult initDevice(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    XrPhysicalDevice *gpus = VK_NULL_HANDLE;
    uint32_t gpuCount = 0;
    uint32_t lastRank = 0;
    int32_t selectedGpuIndex = -1;

    vkResult = vkEnumeratePhysicalDevices(context->instance, &gpuCount, VK_NULL_HANDLE);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    if (gpuCount == 0)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return vkResult;
    }

    gpus = (XrPhysicalDevice *)malloc(sizeof(XrPhysicalDevice) * gpuCount);
    memset((void *)gpus, 0, sizeof(XrPhysicalDevice) * gpuCount);

    VkPhysicalDevice *deviceList = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpuCount);
    memset((void *)deviceList, 0, sizeof(VkPhysicalDevice) * gpuCount);

    vkResult = vkEnumeratePhysicalDevices(context->instance, &gpuCount, deviceList);
    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(gpus);
        XR_FREE(deviceList);
        return vkResult;
    }

    XR_LOG_INFO(context->logger, "Total GPU Found: %d", gpuCount);
    for (uint32_t counter = 0; counter < gpuCount; ++counter)
    {
        XrPhysicalDevice *nextGpu = &gpus[counter];
        nextGpu->gpu = deviceList[counter];
        nextGpu->properties = {};
        nextGpu->memoryProperties = {};

        vkGetPhysicalDeviceProperties(nextGpu->gpu, &(nextGpu->properties));
        vkGetPhysicalDeviceMemoryProperties((nextGpu->gpu), &(nextGpu->memoryProperties));

        findSuitableDeviceQueues(context, nextGpu);
        findMaxMSAASampleCount(nextGpu);
        rankDevice(context, nextGpu);
        printGpuProperties(context, nextGpu, counter + 1, gpuCount);

        if (lastRank < nextGpu->rank)
        {
            lastRank = nextGpu->rank;
            selectedGpuIndex = counter;
        }
    }

    if (selectedGpuIndex > -1)
    {
        XrPhysicalDevice *nextGpuDetails = &gpus[selectedGpuIndex];
        context->gpu = (XrPhysicalDevice *)malloc(sizeof(XrPhysicalDevice));
        memset((void *)context->gpu, 0, sizeof(XrPhysicalDevice));
        memcpy((void *)context->gpu, nextGpuDetails, sizeof(XrPhysicalDevice));
    }

    XR_FREE(gpus);
    XR_FREE(deviceList);

    if (!context->gpu)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return vkResult;
    }

    XR_LOG_INFO(context->logger, "---------- Selected GPU Properties ----------");

    printGpuProperties(context, context->gpu, selectedGpuIndex + 1, gpuCount);

    XR_LOG_INFO(context->logger, "---------- Selected GPU Properties End ----------");

    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);
        VkLayerProperties *layerProperties = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * layerCount);
        memset((void *)layerProperties, 0, sizeof(VkLayerProperties) * layerCount);

        vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);
        xrPrintInstanceLayerProperties(context, layerProperties, layerCount);
        XR_FREE(layerProperties);
    }

    {
        uint32_t layerCount = 0;
        vkEnumerateDeviceLayerProperties(context->gpu->gpu, &layerCount, VK_NULL_HANDLE);
        VkLayerProperties *layerProperties = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * layerCount);
        memset((void *)layerProperties, 0, sizeof(VkLayerProperties) * layerCount);

        vkEnumerateDeviceLayerProperties(context->gpu->gpu, &layerCount, layerProperties);
        xrPrintDeviceLayerProperties(context, layerProperties, layerCount);
        XR_FREE(layerProperties);
    }

    return vkResult;
}

VkResult initializeVulkan(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;
    context->clearValueCount = 2;
    context->clearValue = (VkClearValue *)malloc(sizeof(VkClearValue) * context->clearValueCount);
    memset((void *)context->clearValue, 0, sizeof(VkClearValue) * context->clearValueCount);

    context->clearValue[0].color = {0.0f, 0.0f, 0.0f, 1.0f}; // {r, g, b, a}
    context->clearValue[1].depthStencil = {1.0f, 0};         // {depth, stencil}

#ifdef XR_ENABLE_RUNTIME_DEBUG

    context->enableValidations = VK_TRUE;

    VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

#ifdef XR_ENABLE_DEBUG_REPORT_VERBOSE_BIT

    debugReportFlags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;

#endif

#ifdef XR_ENABLE_DEBUG_REPORT_VERBOSE_BIT

    debugReportFlags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

#endif

    vkResult = xrSetupLayersAndExtensions(context);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkApplicationInfo applicationInfo;
    memset((void *)&applicationInfo, 0, sizeof(VkApplicationInfo));
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = VK_NULL_HANDLE;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = "Vulkan";
    applicationInfo.pEngineName = VK_NULL_HANDLE;
    applicationInfo.engineVersion = 0;

    vkResult = xrInitInstance(context, &applicationInfo);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrCreateDebugger(context, debugReportFlags);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

#endif

    vkResult = initPlatformSpecificSurface(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = initDevice(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitLogicalDevice(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitSwapchain(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitSwapchainImageViews(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDepthStencilImage(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitMSAAColorImage(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitCommandPool(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitFrameBuffers(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitRenderPass(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    {
        homeModel = (XrModel *)malloc(sizeof(XrModel));
        memset((void *)homeModel, 0, sizeof(XrModel));

        homeModel->texture = (XrTexture *)malloc(sizeof(XrTexture));
        memset((void *)homeModel->texture, 0, sizeof(XrTexture));

        homeModel->draw = updateHomeModel;

        VkBool32 homeModelLoaded = xrLoadModal(context, "../resources/models/chalet", "../resources/models/chalet/chalet.obj", homeModel);

        if (homeModelLoaded != VK_TRUE)
        {
            XR_LOG_ERROR(context->logger, "Not able to load home model.");
        }
        else
        {
            XR_LOG_INFO(context->logger, "Home model loaded");
        }

        stbi_uc *homeTextureData = VK_NULL_HANDLE;
        xrLoadTexture(context, "../resources/models/chalet/chalet.jpg", homeModel->texture, &homeTextureData);

        if (!homeTextureData)
        {
            XR_LOG_ERROR(context->logger, "Not able to load home texture.");
        }

        vkResult = xrInitTextureImage(context, homeModel->texture, homeTextureData);

        // Free the texture data as no longer required
        XR_FREE(homeTextureData);

        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        XR_LOG_INFO(context->logger, "Home texture loaded");

        vkResult = xrInitTextureImageView(context, homeModel->texture);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitTextureSampler(context, homeModel->texture);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitVertexBuffer(context, homeModel);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitIndexBuffer(context, homeModel);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitUniformBuffers(context, homeModel);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    {
        vikingRoomModel = (XrModel *)malloc(sizeof(XrModel));
        memset((void *)vikingRoomModel, 0, sizeof(XrModel));

        vikingRoomModel->texture = (XrTexture *)malloc(sizeof(XrTexture));
        memset((void *)vikingRoomModel->texture, 0, sizeof(XrTexture));

        vikingRoomModel->draw = updateVikingRoomModel;

        VkBool32 vikingRoomModelLoaded =
            xrLoadModal(context, "../resources/models/vikingRoom", "../resources/models/vikingRoom/viking_room.obj", vikingRoomModel);

        if (vikingRoomModelLoaded != VK_TRUE)
        {
            XR_LOG_ERROR(context->logger, "Not able to load viking room model.");
        }
        else
        {
            XR_LOG_INFO(context->logger, "Viking room model loaded");
        }

        stbi_uc *vikingRoomTextureData = VK_NULL_HANDLE;
        xrLoadTexture(context, "../resources/models/vikingRoom/viking_room.png", vikingRoomModel->texture, &vikingRoomTextureData);

        if (!vikingRoomTextureData)
        {
            XR_LOG_ERROR(context->logger, "Not able to load viking room texture");
        }

        vkResult = xrInitTextureImage(context, vikingRoomModel->texture, vikingRoomTextureData);

        // Free the texture data as no longer required
        XR_FREE(vikingRoomTextureData);

        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        XR_LOG_INFO(context->logger, "Viking room texture loaded");

        vkResult = xrInitTextureImageView(context, vikingRoomModel->texture);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitTextureSampler(context, vikingRoomModel->texture);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitVertexBuffer(context, vikingRoomModel);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitIndexBuffer(context, vikingRoomModel);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = xrInitUniformBuffers(context, vikingRoomModel);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        models = (XrModel **)malloc(sizeof(XrModel *) * modelsCount);
        memset((void *)models, 0, sizeof(XrModel *) * modelsCount);

        models[0] = homeModel;
        models[1] = vikingRoomModel;
    }

    vkResult = xrCreateShaderModule(context, "../shaders/vert.spv", &(context->vertexShaderModule));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrCreateShaderModule(context, "../shaders/frag.spv", &(context->fragmentShaderModule));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDescriptorSetLayout(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDescriptorPool(context, modelsCount);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDescriptorSets(context, models, modelsCount);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitGraphicsPiplineCache(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitGraphicsPipline(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitSynchronizations(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitCommandBuffers(context, models, modelsCount);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

void cleanUp(XrContext *context)
{
    XR_LOG_INFO(context->logger, "---------- Cleanup started ----------");

    if (isFullscreen)
    {
        isFullscreen = false;
        toggleFullscreen(isFullscreen);
    }

    xrWaitForIdle(context);
    xrDestroySynchronizations(context);
    xrDestroyCommandBuffers(context);
    xrDestroyDescriptorSets(context, models, modelsCount);
    xrDestroyDescriptorPool(context);

    xrDestroyUniformBuffers(context, homeModel);
    xrDestroyIndexBuffer(context, homeModel);
    xrDestroyVertexBuffer(context, homeModel);
    xrDestroyTextureSampler(context, homeModel->texture);
    xrDestroyTextureImageView(context, homeModel->texture);
    xrDestroyTextureImage(context, homeModel->texture);

    xrDestroyUniformBuffers(context, vikingRoomModel);
    xrDestroyIndexBuffer(context, vikingRoomModel);
    xrDestroyVertexBuffer(context, vikingRoomModel);
    xrDestroyTextureSampler(context, vikingRoomModel->texture);
    xrDestroyTextureImageView(context, vikingRoomModel->texture);
    xrDestroyTextureImage(context, vikingRoomModel->texture);

    xrDestroyFrameBuffers(context);
    xrDestroyMSAAColorImage(context);
    xrDestroyDepthStencilImage(context);
    xrDestroyCommandPool(context);
    xrDestroyGraphicsPipline(context);
    xrDestroyGraphicsPiplineCache(context);
    xrDestroyShaderModule(context, &(context->vertexShaderModule));
    xrDestroyShaderModule(context, &(context->fragmentShaderModule));
    xrDestroyDescriptorSetLayout(context);
    xrDestroyRenderPass(context);
    xrDestroySwapchainImageViews(context);
    xrDestroySwapchain(context);
    xrDestroyDevice(context);

    // The surface need to be destroyed before instance is deleted.
    destroyPlatformSpecificSurface(context);

    xrDestroyDebugger(context);
    xrDestroyInstance(context);

    XR_FREE(vikingRoomModel->texture->image);
    XR_FREE(vikingRoomModel->texture);
    XR_FREE(vikingRoomModel->vertices);
    XR_FREE(vikingRoomModel->vertexIndices);
    XR_FREE(vikingRoomModel->material);
    XR_FREE(vikingRoomModel);

    XR_FREE(homeModel->texture->image);
    XR_FREE(homeModel->texture);
    XR_FREE(homeModel->vertices);
    XR_FREE(homeModel->vertexIndices);
    XR_FREE(homeModel->material);
    XR_FREE(homeModel);

    XR_FREE(models);
    XR_FREE(context->clearValue);
    XR_FREE(context->gpu);

    XR_LOG_INFO(context->logger, "---------- Cleanup done ----------");

    xrDestroyLogger(&(context->logger));
    XR_FREE(context);
}

void updateHomeModel(XrUniformBuffer *ubo)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, -1.0f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // To push object deep into screen, modify the eye matrix to have more positive (greater) value at z-axis.
    ubo->model = translationMatrix * rotationMatrix;
    ubo->view = glm::lookAt(glm::vec3(6.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo->projection = glm::perspective(glm::radians(45.0f), (float)context->surfaceExtent.width / (float)context->surfaceExtent.height, 0.1f, 100.0f);

    // The GLM is designed for OpenGL, where the Y coordinate of the clip coordinate is inverted.
    // If we do not fix this then the image will be rendered upside-down.
    // The easy way to fix this is to flip the sign on the scaling factor of Y axis
    // in the projection matrix.
    ubo->projection[1][1] *= -1.0f;

    // Lights
    ubo->lightAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    ubo->lightDiffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ubo->lightSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ubo->lightPosition = glm::vec4(100.0f, 100.0f, 100.0f, 1.0f);

    // Material
    ubo->materialAmbient = homeModel->material->ambient;
    ubo->materialDiffuse = homeModel->material->diffuse;
    ubo->materialSpecular = homeModel->material->specular;
    ubo->materialShininess = homeModel->material->shininess;
}

void updateVikingRoomModel(XrUniformBuffer *ubo)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 1.5f, -1.0f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // To push object deep into screen, modify the eye matrix to have more positive (greater) value at z-axis.
    ubo->model = translationMatrix * rotationMatrix;
    ubo->view = glm::lookAt(glm::vec3(6.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo->projection = glm::perspective(glm::radians(45.0f), (float)context->surfaceExtent.width / (float)context->surfaceExtent.height, 0.1f, 100.0f);

    // The GLM is designed for OpenGL, where the Y coordinate of the clip coordinate is inverted.
    // If we do not fix this then the image will be rendered upside-down.
    // The easy way to fix this is to flip the sign on the scaling factor of Y axis
    // in the projection matrix.
    ubo->projection[1][1] *= -1.0f;

    // Lights
    ubo->lightAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    ubo->lightDiffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ubo->lightSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ubo->lightPosition = glm::vec4(100.0f, 100.0f, 100.0f, 1.0f);

    // Material
    ubo->materialAmbient = vikingRoomModel->material->ambient;
    ubo->materialDiffuse = vikingRoomModel->material->diffuse;
    ubo->materialSpecular = vikingRoomModel->material->specular;
    ubo->materialShininess = vikingRoomModel->material->shininess;
}

int mainLoop(XrContext *context)
{
    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;
    std::wstring fpsTitle = L"";

    while (isRunning)
    {
        isRunning = !(isCloseButtonClicked || isEscapeKeyPressed);

        xcb_generic_event_t *event = NULL;
        while ((event = xcb_poll_for_event(xcbConnection)))
        {
            handleEvent(event);
            free(event);
        }

        ++frameCounter;

        if (lastTime + std::chrono::seconds(1) < timer.now())
        {
            lastTime = timer.now();
            fps = frameCounter;
            frameCounter = 0;
            fpsTitle.assign(windowTitle.begin(), windowTitle.end());
            fpsTitle.append(L" | FPS - " + std::to_wstring(fps));

            xcb_change_property(
                xcbConnection,
                XCB_PROP_MODE_REPLACE,
                xcbWindow,
                XCB_ATOM_WM_NAME,
                XCB_ATOM_STRING,
                8,
                std::string(fpsTitle.begin(), fpsTitle.end()).size(),
                std::string(fpsTitle.begin(), fpsTitle.end()).c_str()
            );

            xcb_flush(xcbConnection);
        }

        render();
    }

    return EXIT_SUCCESS;
}

void render()
{
    VkResult vkResult = xrRender(context, models, modelsCount);

    // Recreate the swap chain if vkResult is suboptimal or out of data because we want the best possible vkResult.
    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        XR_LOG_INFO(context->logger, "Swapchain out of date");

        vkResult = xrRecreateSwapChain(context, models, modelsCount);

        if (XR_IS_ERROR(vkResult))
        {
            XR_LOG_ERROR(context->logger, "Failed to recreate out of date swapchain");
        }

        return;
    }

    if (vkResult == VK_SUBOPTIMAL_KHR)
    {
        XR_LOG_INFO(context->logger, "Swapchain suboptimal");
        vkResult = xrRecreateSwapChain(context, models, modelsCount);

        if (XR_IS_ERROR(vkResult))
        {
            XR_LOG_ERROR(context->logger, "Failed to recreate suboptimal swapchain");
        }

        return;
    }

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to render");
    }
}

VkResult initPlatformSpecificSurface(XrContext *context)
{
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = VK_NULL_HANDLE;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.connection = xcbConnection;
    surfaceCreateInfo.window = xcbWindow;

    return vkCreateXcbSurfaceKHR(context->instance, &surfaceCreateInfo, VK_NULL_HANDLE, &(context->surface));
}

void destroyPlatformSpecificSurface(XrContext *context)
{
    vkDestroySurfaceKHR(context->instance, context->surface, VK_NULL_HANDLE);
    context->surface = VK_NULL_HANDLE;
}

void resize(uint32_t width, uint32_t height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (width == context->surfaceExtent.width && height == context->surfaceExtent.height)
    {
        return;
    }

    context->surfaceExtent.width = width;
    context->surfaceExtent.height = height;

    if (context)
    {
        xrRecreateSwapChain(context, models, modelsCount);
    }
}

void toggleFullscreen(bool isFullscreen)
{
    if (xcbWindow)
    {
        xcb_intern_atom_cookie_t atom_wm_state_normal_cookie = xcb_intern_atom(xcbConnection, 0, std::strlen("_NET_WM_STATE"), "_NET_WM_STATE");
        xcb_intern_atom_reply_t *atom_wm_state_normal_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_state_normal_cookie, NULL);

        xcb_intern_atom_cookie_t atom_wm_state_fullscreen_cookie =
            xcb_intern_atom(xcbConnection, 0, strlen("_NET_WM_STATE_FULLSCREEN"), "_NET_WM_STATE_FULLSCREEN");
        xcb_intern_atom_reply_t *atom_wm_state_fullscreen_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_state_fullscreen_cookie, NULL);

        xcb_client_message_event_t event = {};
        memset(&event, 0, sizeof(event));
        event.response_type = XCB_CLIENT_MESSAGE;
        event.window = xcbWindow;
        event.type = atom_wm_state_normal_reply->atom;
        event.format = 32;
        event.data.data32[0] = isFullscreen ? 1 : 0;
        event.data.data32[1] = atom_wm_state_fullscreen_reply->atom;

        xcb_send_event(xcbConnection, 0, xcbScreen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *)(&event));
        xcb_flush(xcbConnection);

        free(atom_wm_state_normal_reply);
        free(atom_wm_state_fullscreen_reply);
    }
}

#endif // VK_USE_PLATFORM_XCB_KHR
