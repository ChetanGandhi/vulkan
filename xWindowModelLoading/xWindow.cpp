#include "platform.h"

#if defined(VK_USE_PLATFORM_XCB_KHR)

#include "vulkanWindow.h"

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

        case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
            switch(keyEvent->detail)
            {
                case 0x9: // Escape key code.
                    isEscapeKeyPressed = true;
                break;

                default:
                break;
            }
        }
        break;

        case XCB_DESTROY_NOTIFY:
            isCloseButtonClicked = true;
        break;

        default:
        break;
    }
}

int main(void)
{
    Logger::init("debug_linux.log");

    windowName = "VulkanWindow";
    windowTitle = "Vulkan Window | XWindows";

    surfaceSize.width = 800;
    surfaceSize.height = 600;

    initializePlatformSpecificWindow();

    int returnCode = mainLoop();

    cleanUp();
    Logger::close();

    return returnCode;
}

void initializePlatformSpecificWindow()
{
    assert(surfaceSize.width > 0);
    assert(surfaceSize.height > 0);

    uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t valueList[32] = {0};

    const xcb_setup_t *setup = NULL;
    xcb_screen_iterator_t screenIterator;
    int screenCount = 0;

    xcbConnection = xcb_connect(NULL, &screenCount);

    if(xcbConnection == NULL)
    {
        printf("Error: Cannot create connection to XCB\n");
        LOG("Error: Cannot create connection to XCB\n");
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

    xcb_create_window(xcbConnection,
        XCB_COPY_FROM_PARENT,
        xcbWindow, xcbScreen->root,
        0, 0, surfaceSize.width, surfaceSize.height, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        xcbScreen->root_visual,
        valueMask, valueList
    );

    // Enable the window close button action.
    xcb_intern_atom_cookie_t atom_wm_protocols_cookie = xcb_intern_atom(xcbConnection, true, std::strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *atom_wm_protocols_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_protocols_cookie, NULL);

    xcb_intern_atom_cookie_t atom_wm_delete_window_cookie = xcb_intern_atom(xcbConnection, false, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    atom_wm_delete_window_reply = xcb_intern_atom_reply(xcbConnection, atom_wm_delete_window_cookie, NULL);

    xcb_change_property(xcbConnection,
        XCB_PROP_MODE_REPLACE,
        xcbWindow,
        atom_wm_protocols_reply->atom,
        4,
        32,
        1,
        &(atom_wm_delete_window_reply->atom)
    );

    // Update the window title.
    xcb_change_property(xcbConnection,
        XCB_PROP_MODE_REPLACE,
        xcbWindow, XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        windowTitle.size(),
        windowTitle.c_str()
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

}

void cleanUp()
{
    LOG("---------- Cleanup Started ----------");

    if(isFullscreen)
    {
        isFullscreen = false;
        toggleFullscreen(isFullscreen);
    }

    destroyPlatformSpecificWindow();
}

int mainLoop()
{
    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;
    std::string fpsTitle = "";

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
            fpsTitle = windowTitle + " | FPS - " + std::to_string(fps);
            xcb_change_property(xcbConnection,
                XCB_PROP_MODE_REPLACE,
                xcbWindow, XCB_ATOM_WM_NAME,
                XCB_ATOM_STRING,
                8,
                fpsTitle.size(),
                fpsTitle.c_str()
            );
            xcb_flush(xcbConnection);
        }
    }

    return EXIT_SUCCESS;
}

void initPlatformSpecificSurface()
{

}

void destroyPlatformSpecificSurface()
{

}

void resize(uint32_t width, uint32_t height)
{
    if(width == 0 || height == 0)
    {
        return;
    }

    surfaceSize.width = width;
    surfaceSize.height = height;
}

void toggleFullscreen(bool isFullscreen)
{

}

#endif // VK_USE_PLATFORM_XCB_KHR
