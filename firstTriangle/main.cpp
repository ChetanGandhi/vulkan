#include "vulkanWindow.h"

int main()
{
    VulkanWindow *window = new VulkanWindow(800, 600, "VulkanWindow", "Vulkan Window");

    while(window->run())
    {
        // CPU Logic

        // Render screen

        window->render();
    }

    delete window;

    return 0;
}
