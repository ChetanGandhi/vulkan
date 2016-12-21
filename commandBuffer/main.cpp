#include "renderer.h"

int main()
{
    Renderer renderer;

    VkDevice device = renderer.device;
    VkQueue queue = renderer.queue;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffers[2];
    // VkFence fence = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;

    // VkFenceCreateInfo fenceCreateInfo {};
    // fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // fenceCreateInfo.flags = 0;
    // fenceCreateInfo.pNext = nullptr;

    // vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

    VkSemaphoreCreateInfo semaphoreCreateInfo {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);

    VkCommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.queueFamilyIndex = renderer.graphicsFamilyIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = sizeof(commandBuffers)/sizeof(commandBuffers[0]);
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.pNext = nullptr;

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers);

    {
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.pInheritanceInfo = NULL;

        vkBeginCommandBuffer(commandBuffers[0], &commandBufferBeginInfo);

        vkCmdPipelineBarrier(commandBuffers[0],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            0, nullptr);

        VkViewport viewport {};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 512.0f;
        viewport.height = 512.0f;

        vkCmdSetViewport(commandBuffers[0], 0, 1, &viewport);
        vkEndCommandBuffer(commandBuffers[0]);
    }

    {
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = NULL;

        vkBeginCommandBuffer(commandBuffers[1], &commandBufferBeginInfo);

        VkViewport viewport {};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 512.0f;
        viewport.height = 512.0f;

        vkCmdSetViewport(commandBuffers[1], 0, 1, &viewport);
        vkEndCommandBuffer(commandBuffers[1]);
    }

    {
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = NULL;
        submitInfo.pWaitDstStageMask = NULL;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[0];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &semaphore;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    {
        VkPipelineStageFlags pipelineStageFlags[] {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &semaphore;
        submitInfo.pWaitDstStageMask = pipelineStageFlags;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[1];
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = NULL;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    vkQueueWaitIdle(queue);
    // vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    // vkDestroyFence(device, fence, nullptr);
    vkDestroySemaphore(device, semaphore, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);

    return 0;
}
