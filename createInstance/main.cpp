#include "renderer.h"

int main()
{
    Renderer renderer;

    VkDevice device = renderer.device;
    VkQueue queue = renderer.queue;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    VkFenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    fenceCreateInfo.pNext = NULL;

    vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

    VkCommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = NULL;
    commandPoolCreateInfo.queueFamilyIndex = renderer.graphicsFamilyIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.pNext = NULL;

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pNext = NULL;
    commandBufferBeginInfo.pInheritanceInfo = NULL;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    VkViewport viewport {};
    viewport.maxDepth = 1.0f;
    viewport.minDepth = 0.0f;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 512.0f;
    viewport.height = 512.0f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;

    vkQueueSubmit(queue, 1, &submitInfo, fence);
    // vkQueueWaitIdle(queue);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);

    return 0;
}
