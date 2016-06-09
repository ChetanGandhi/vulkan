#include "renderer.h"

int main()
{
    Renderer renderer;

    renderer.createVulkanVindow(800, 600, "Vulkan Window");

    while(renderer.run())
    {

    }

    return 0;
}
