#pragma once

#include <vector>
#include <array>
#include <string>
#include <glm/glm.hpp>

#include "platform.h"
#include "common.h"
#include "vertex.h"

class Renderer
{

public:
    Renderer(SurfaceSize surfaceSize);
    ~Renderer();

    void setSurfaceSize(SurfaceSize surfaceSize);
    void setSurface(VkSurfaceKHR surface);
    void waitForIdle();

    void initDevice();
    void initLogicalDevice();
    void destroyDevice();

    void initSwapchain();
    void destroySwapchain();

    void initSwapchainImageViews();
    void destroySwapchainImageViews();

    void initRenderPass();
    void destroyRenderPass();

    void initDescriptorSetLayout();
    void destroyDescriptorSetLayout();

    void initGraphicsPipline();
    void destroyGraphicsPipline();

    void initFrameBuffers();
    void destroyFrameBuffers();

    void initCommandPool();
    void destroyCommandPool();

    void initDepthStencilImage();
    void destoryDepthStencilImage();

    void initTextureImage();
    void destroyTextureImage();

    void initTextureImageView();
    void destroyTextureImageView();

    void initTextureSampler();
    void destoryTextureSampler();

    void loadModel();

    void initVertexBuffer();
    void destroyVertexBuffer();

    void initIndexBuffer();
    void destroyIndexBuffer();

    void initUniformBuffer();
    void destroyUniformBuffer();

    void initDescriptorPool();
    void destroyDescriptorPool();

    void initDescriptorSet();
    void destroyDescriptorSet();

    void initCommandBuffers();
    void destroyCommandBuffers();

    void initSynchronizations();
    void destroySynchronizations();

    void recreateSwapChain();
    void cleanupSwapChain();

    void render();
    void updateUniformBuffer();

    const VkInstance getVulkanInstance() const;

private:
    SurfaceSize surfaceSize;

    const std::string chaletModelResourcePath = "resources/models/chalet/chalet.obj";
    const std::string chaletTextureResourcePath = "resources/textures/chalet/chalet.jpg";

    VkInstance            instance                = VK_NULL_HANDLE;
    VkDevice              device                  = VK_NULL_HANDLE;
    VkSurfaceKHR          surface                 = VK_NULL_HANDLE;
    VkQueue               graphicsQueue           = VK_NULL_HANDLE;
    VkQueue               presentQueue            = VK_NULL_HANDLE;
    VkBuffer              vertexBuffer            = VK_NULL_HANDLE;
    VkSwapchainKHR        swapchain               = VK_NULL_HANDLE;
    VkRenderPass          renderPass              = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout     = VK_NULL_HANDLE;
    VkPipelineLayout      pipelineLayout          = VK_NULL_HANDLE;
    VkPipeline            pipeline                = VK_NULL_HANDLE;
    VkCommandPool         commandPool             = VK_NULL_HANDLE;
    VkSemaphore           imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore           renderFinishedSemaphore = VK_NULL_HANDLE;
    VkDeviceMemory        vertexBufferMemory      = VK_NULL_HANDLE;
    VkBuffer              indexBuffer             = VK_NULL_HANDLE;
    VkDeviceMemory        indexBufferMemory       = VK_NULL_HANDLE;
    VkBuffer              uniformBuffer           = VK_NULL_HANDLE;
    VkDeviceMemory        uniformBufferMemory     = VK_NULL_HANDLE;
    VkDescriptorPool      descriptorPool          = VK_NULL_HANDLE;
    VkDescriptorSet       descriptorSet           = VK_NULL_HANDLE;
    VkImage               depthImage              = VK_NULL_HANDLE;
    VkDeviceMemory        depthImageMemory        = VK_NULL_HANDLE;
    VkImageView           depthImageView          = VK_NULL_HANDLE;
    VkImage               textureImage            = VK_NULL_HANDLE;
    VkDeviceMemory        textureImageMemory      = VK_NULL_HANDLE;
    VkImageView           textureImageView        = VK_NULL_HANDLE;
    VkSampler             textureSampler          = VK_NULL_HANDLE;

    struct GpuDetails {
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
    } gpuDetails;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    QueueFamilyIndices queueFamilyIndices;

    std::vector<const char*> instanceLayerList;
    std::vector<const char*> deviceLayerList;
    std::vector<const char*> instanceExtensionList;
    std::vector<const char*> deviceExtensionList;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> vertexIndices;

    uint32_t swapchainImageCount = 2;

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    } swapchainSupportDetails;

    VkSurfaceFormatKHR surfaceFormat = {};

    void setupDebugLayer();
    void setupLayersAndExtensions();

    void enableDebug();
    void disableDebug();

    void initInstance();
    void destroyInstance();

    void beginOneTimeCommand(VkCommandBuffer &commandBuffer);
    void endOneTimeCommand(VkCommandBuffer &commandBuffer);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);
    void listAllPhysicalDevices(std::vector<GpuDetails> *gpuDetailsList);
    void querySwapchainSupportDetails(VkPhysicalDevice gpu, SwapchainSupportDetails *details);

    VkFormat findSupportedFormat(VkPhysicalDevice gpu, const std::vector<VkFormat> &formatsToCheck, VkImageTiling imageTiling, VkFormatFeatureFlags formatFeatureFlags);
    VkFormat findDepthFormat();
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> presentModes);
    void chooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent);

    bool isDeviceSuitable(VkPhysicalDevice gpu);
    bool findSuitableDeviceQueues(VkPhysicalDevice gpu, QueueFamilyIndices *queueFamilyIndices);
    bool checkDeviceExtensionSupport(VkPhysicalDevice gpu);
    bool hasStencilComponent(VkFormat format);

    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling imageTiling, VkImageUsageFlags imageUsage, VkMemoryPropertyFlags memoryPropertyFlags, VkImage &image, VkDeviceMemory &imageMemory);
    void createImageView(VkImage image, VkFormat format, VkImageView &imageView, VkImageAspectFlags imageAspectFlags);
    void copyBuffer(VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // Debug methods

    void printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount);
    void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
    void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
    void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
    void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
};
