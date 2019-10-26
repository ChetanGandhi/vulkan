#include <xRenderer/platform.h>

#if defined(VK_USE_PLATFORM_XCB_KHR)

#include <xRenderer/vulkanWindow.h>

void handleEvent(const xcb_generic_event_t *event)
{
    switch(event->response_type & 0x7f)
    {
        case XCB_CLIENT_MESSAGE:
        {
            const xcb_client_message_event_t *clientMessageEvent = (xcb_client_message_event_t *)event;
            if(clientMessageEvent->data.data32[0] == atom_wm_delete_window_reply->atom)
            {
                isCloseButtonClicked = true;
            }
        }
        break;

        case XCB_KEY_PRESS:
        {
            const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
            switch(keyEvent->detail)
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
    xr::Logger::initialize("debug_linux.log");

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | XWindows";

    vkState = new xr::VulkanState();
    vkState->surfaceSize = {};
    vkState->surfaceSize.width = 800;
    vkState->surfaceSize.height = 600;
    vkState->vertexShaderFilePath = "../shaders/vert.spv";
    vkState->fragmentShaderFile  ="../shaders/frag.spv";

    initializePlatformSpecificWindow();
    initializeVulkan();

    int returnCode = mainLoop();

    cleanUp();
    xr::Logger::close();

    return returnCode;
}

void initializePlatformSpecificWindow()
{
    uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t valueList[32] = {0};

    const xcb_setup_t *setup = NULL;
    xcb_screen_iterator_t screenIterator;
    xcb_generic_error_t *error = NULL;
    xcb_void_cookie_t cookieForError;
    int screenCount = 0;

    xcbConnection = xcb_connect(NULL, &screenCount);

    if(xcbConnection == NULL)
    {
        printf("Error: Cannot create connection to XCB\n");
        logf("Error: Cannot create connection to XCB\n");
        std::exit(EXIT_FAILURE);
    }

    setup = xcb_get_setup(xcbConnection);
    screenIterator = xcb_setup_roots_iterator(setup);

    while(screenCount > 0)
    {
        --screenCount;
        xcb_screen_next(&screenIterator);
    }

    xcbScreen = screenIterator.data;
    xcbWindow = xcb_generate_id(xcbConnection);

    valueList[0] = xcbScreen->black_pixel;
    valueList[1] = XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_KEY_PRESS
    | XCB_EVENT_MASK_EXPOSURE
    | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    | XCB_EVENT_MASK_POINTER_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS
    | XCB_EVENT_MASK_BUTTON_RELEASE;

    cookieForError = xcb_create_window(
        xcbConnection,
        XCB_COPY_FROM_PARENT,
        xcbWindow, xcbScreen->root,
        0, 0, vkState->surfaceSize.width, vkState->surfaceSize.height, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        xcbScreen->root_visual,
        valueMask, valueList
    );

    error = xcb_request_check(xcbConnection, cookieForError);

    if (error)
    {
        logf("Error: Cannot create window [%d]", error->error_code);
        printf("Error: Cannot create window [%d]\n", error->error_code);
        free (error);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    // Enable the window close button action.
    xcb_intern_atom_cookie_t atom_wm_protocols_cookie = xcb_intern_atom(xcbConnection, 1, std::strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *atom_wm_protocols_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_protocols_cookie, NULL);

    xcb_intern_atom_cookie_t atom_wm_delete_window_cookie = xcb_intern_atom(xcbConnection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    atom_wm_delete_window_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_delete_window_cookie, NULL);

    xcb_change_property(
        xcbConnection,
        XCB_PROP_MODE_REPLACE,
        xcbWindow,
        atom_wm_protocols_reply->atom,
        4,
        32,
        1,
        &(atom_wm_delete_window_reply->atom)
    );

    // Update the window title.
    xcb_change_property(
        xcbConnection,
        XCB_PROP_MODE_REPLACE,
        xcbWindow, XCB_ATOM_WM_NAME,
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
    if(xcbWindow)
    {
        xcb_destroy_window(xcbConnection, xcbWindow);
    }

    if(xcbConnection)
    {
        xcb_disconnect(xcbConnection);
        xcbConnection = nullptr;
    }
}

void initializeVulkan()
{
    renderer = new xr::Renderer(vkState);

    initPlatformSpecificSurface(&(vkState->instance->vkInstance), &(vkState->surface));

    renderer->initDevice();
    renderer->initLogicalDevice();
    renderer->initSwapchain();
    renderer->initSwapchainImageViews();
    renderer->initRenderPass();
    renderer->initDescriptorSetLayout();
    renderer->initGraphicsPiplineCache();
    renderer->initGraphicsPipline();
    renderer->initCommandPool();
    renderer->initDepthStencilImage();
    renderer->initMSAAColorImage();
    renderer->initFrameBuffers();
    renderer->initTextureImage("../resources/textures/chalet/chalet.jpg");
    renderer->initTextureImageView();
    renderer->initTextureSampler();
    renderer->loadModel("../resources/models/chalet/chalet.obj");
    renderer->initVertexBuffer();
    renderer->initIndexBuffer();
    renderer->initUniformBuffers();
    renderer->initDescriptorPool();
    renderer->initDescriptorSets();
    renderer->initCommandBuffers();
    renderer->initSynchronizations();
}

void cleanUp()
{
    logf("---------- Cleanup started ----------");

    if(isFullscreen)
    {
        isFullscreen = false;
        toggleFullscreen(isFullscreen);
    }

    if(renderer != nullptr)
    {
        renderer->waitForIdle();
        renderer->destroySynchronizations();
        renderer->destroyCommandBuffers();
        renderer->destroyDescriptorSets();
        renderer->destroyDescriptorPool();
        renderer->destroyUniformBuffers();
        renderer->destroyIndexBuffer();
        renderer->destroyVertexBuffer();
        renderer->destoryTextureSampler();
        renderer->destroyTextureImageView();
        renderer->destroyTextureImage();
        renderer->destroyFrameBuffers();
        renderer->destoryMSAAColorImage();
        renderer->destoryDepthStencilImage();
        renderer->destroyCommandPool();
        renderer->destroyGraphicsPipline();
        renderer->destroyGraphicsPiplineCache();
        renderer->destroyDescriptorSetLayout();
        renderer->destroyRenderPass();
        renderer->destroySwapchainImageViews();
        renderer->destroySwapchain();
        renderer->destroyDevice();
    }

    // The surface need to be destoyed before instance is deleted.
    destroyPlatformSpecificSurface();

    // Instance is deleted in destructor of Renderer class.
    delete renderer;
    renderer = nullptr;

    delete vkState;
    vkState = nullptr;

    destroyPlatformSpecificWindow();

    logf("---------- Cleanup done ----------");
}

int mainLoop()
{
    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;
    std::wstring fpsTitle = L"";

    while(isRunning)
    {
        isRunning = !(isCloseButtonClicked || isEscapeKeyPressed);

        xcb_generic_event_t *event = NULL;
        while((event = xcb_poll_for_event(xcbConnection)))
        {
            handleEvent(event);
            free(event);
        }

        ++frameCounter;

        if(lastTime + std::chrono::seconds(1) < timer.now())
        {
            lastTime = timer.now();
            fps = frameCounter;
            frameCounter = 0;
            fpsTitle.assign(windowTitle.begin(), windowTitle.end());
            fpsTitle.append(L" | FPS - " + std::to_wstring(fps));

            xcb_change_property(xcbConnection,
                XCB_PROP_MODE_REPLACE,
                xcbWindow,
                XCB_ATOM_WM_NAME,
                XCB_ATOM_STRING,
                8,
                fpsTitle.length(),
                fpsTitle.data()
            );

            xcb_flush(xcbConnection);
        }

        // This will also update uniform buffer as per current inflight image.
        renderer->render();
    }

    return EXIT_SUCCESS;
}

void initPlatformSpecificSurface(VkInstance *instance, VkSurfaceKHR *surface)
{
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = nullptr;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.connection = xcbConnection;
    surfaceCreateInfo.window = xcbWindow;

    VkResult result = vkCreateXcbSurfaceKHR(*instance, &surfaceCreateInfo, nullptr, surface);
    CHECK_ERROR(result);
}

void destroyPlatformSpecificSurface()
{
    vkDestroySurfaceKHR(vkState->instance->vkInstance, vkState->surface, nullptr);
    vkState->surface = VK_NULL_HANDLE;
}

void resize(uint32_t width, uint32_t height)
{
    if(width == 0 || height == 0)
    {
        return;
    }

    if(width == vkState->surfaceSize.width && height == vkState->surfaceSize.height)
    {
        return;
    }

    vkState->surfaceSize.width = width;
    vkState->surfaceSize.height = height;

    if(renderer != nullptr)
    {
        renderer->recreateSwapChain();
    }
}

void toggleFullscreen(bool isFullscreen)
{
    if(xcbWindow)
    {
        xcb_intern_atom_cookie_t atom_wm_state_normal_cookie = xcb_intern_atom(xcbConnection, 0, std::strlen("_NET_WM_STATE"), "_NET_WM_STATE");
        xcb_intern_atom_reply_t *atom_wm_state_normal_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_state_normal_cookie, NULL);

        xcb_intern_atom_cookie_t atom_wm_state_fullscreen_cookie = xcb_intern_atom(xcbConnection, 0, strlen("_NET_WM_STATE_FULLSCREEN"), "_NET_WM_STATE_FULLSCREEN");
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
