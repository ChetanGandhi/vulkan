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
    Renderer renderer;

    VulkanWindow *window = renderer.createVulkanVindow(800, 600, "Vulkan Window");

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = renderer.getGraphicsFamilyIndex();
    vkCreateCommandPool(renderer.getVulkanDevice(), &commandPoolCreateInfo, nullptr, &commandPool);

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(renderer.getVulkanDevice(), &commandBufferAllocateInfo, &commandBuffer);

    VkSemaphore renderingCompleteSemaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphoreCreateInfo {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    vkCreateSemaphore(renderer.getVulkanDevice(), &semaphoreCreateInfo, nullptr, &renderingCompleteSemaphore);

    float colorRotator = 0.0f;
    auto timer = std::chrono::steady_clock();
    auto lastTime = timer.now();
    uint64_t frameCounter = 0;
    uint64_t fps = 0;

    while(renderer.run())
    {
        // CPU Logic

        #if ENABLE_FPS

        ++frameCounter;

        if(lastTime + std::chrono::seconds(1) < timer.now())
        {
            lastTime = timer.now();
            fps = frameCounter;
            frameCounter  = 0;
            std::cout<<"FPS: "<<fps<<std::endl;
        }

        #endif // ENABLE_FPS

        // Begin rendering
        window->beginRendering();

        // Record command buffer
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        VkRect2D renderArea {};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent = window->getVulkanSurfaceSize();

        colorRotator += 0.001;

        std::array<VkClearValue, 2> clearValue {};
        clearValue[0].depthStencil.depth = 0.0f;
        clearValue[0].depthStencil.stencil = 0;
        clearValue[1].color.float32[0] = std::sin( colorRotator + CIRCLE_THIRD_1 ) * 0.5 + 0.5;
        clearValue[1].color.float32[1] = std::sin( colorRotator + CIRCLE_THIRD_2 ) * 0.5 + 0.5;
        clearValue[1].color.float32[2] = std::sin( colorRotator + CIRCLE_THIRD_3 ) * 0.5 + 0.5;
        clearValue[1].color.float32[3] = 1.0f;

        VkRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = window->getVulkanRenderPass();
        renderPassBeginInfo.framebuffer = window->getVulkanActiveFramebuffer();
        renderPassBeginInfo.renderArea = renderArea;
        renderPassBeginInfo.clearValueCount = clearValue.size();
        renderPassBeginInfo.pClearValues = clearValue.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdEndRenderPass(commandBuffer);

        vkEndCommandBuffer(commandBuffer);

        // Submit command buffer
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderingCompleteSemaphore;

        VkResult result = vkQueueSubmit(renderer.getVulkanQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        checkError(result);

        // End rendering

        window->endRendering({renderingCompleteSemaphore});
    }

    vkQueueWaitIdle(renderer.getVulkanQueue());
    vkDestroySemaphore(renderer.getVulkanDevice(), renderingCompleteSemaphore, nullptr);
    vkDestroyCommandPool(renderer.getVulkanDevice(), commandPool, nullptr);

    return 0;
}
