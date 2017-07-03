#include "buildParam.h"
#include <array>
#include <chrono>
#include <iostream>

#include "renderer.h"
#include "vulkanWindow.h"
#include "utils.h"

constexpr double PI = 3.14159265358979323846;
constexpr double CIRCLE_RAD = PI * 2;
constexpr double CIRCLE_THIRD = CIRCLE_RAD / 3.0;
constexpr double CIRCLE_THIRD_1 = 0;
constexpr double CIRCLE_THIRD_2 = CIRCLE_THIRD;
constexpr double CIRCLE_THIRD_3 = CIRCLE_THIRD * 2;

int main()
{
    VulkanWindow *window = new VulkanWindow(800, 600, "VulkanWindow", "Vulkan Window");

    // float colorRotator = 0.0f;
    // auto timer = std::chrono::steady_clock();
    // auto lastTime = timer.now();
    // uint64_t frameCounter = 0;
    // uint64_t fps = 0;

    while(window->run())
    {
        // CPU Logic

        // #if ENABLE_FPS

        // ++frameCounter;

        // if(lastTime + std::chrono::seconds(1) < timer.now())
        // {
        //     lastTime = timer.now();
        //     fps = frameCounter;
        //     frameCounter  = 0;
        //     std::cout<<"----- FPS: "<<fps<<" -----"<<std::endl;
        // }

        // #endif // ENABLE_FPS

        // colorRotator += 0.001;

        // Begin rendering

        window->render();
    }

    delete window;

    return 0;
}
