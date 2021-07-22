#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <RapidVulkan/Instance.hpp>
#include <RapidVulkan/Device.hpp>
#include <RapidVulkan/SwapchainKHR.hpp>
#include <RapidVulkan/RenderPass.hpp>
#include <RapidVulkan/ImageView.hpp>
#include <RapidVulkan/Framebuffer.hpp>
#include <RapidVulkan/Semaphore.hpp>
#include <RapidVulkan/ShaderModule.hpp>
#include <RapidVulkan/GraphicsPipeline.hpp>
#include <RapidVulkan/CommandPool.hpp>
#include <RapidVulkan/CommandBuffers.hpp>
#include "IWindow.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace BadgerSandbox
{
    struct uniformBuffer
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo decriptor = {};
        void* mapped = nullptr;
    };

    struct vertexBuffer
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
    };

    struct renderPassResources
    {
        RapidVulkan::ShaderModule vertexShaderModule;
        RapidVulkan::ShaderModule fragmentShaderModule;

        VkDescriptorSetLayout descriptorSetLayout;
        std::vector<VkDescriptorSet> descriptorSets;

        RapidVulkan::GraphicsPipeline graphicsPipeline;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        VkDescriptorSetLayout dsLayout;
        VkDescriptorPool dPool;
        VkDescriptorSet ds;
        VkPipelineLayout pipelineLayout;

        std::vector<uniformBuffer> matrixUniformBuffers;
        std::vector<uniformBuffer> lightUniformBuffer;

        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        //Mats actually sent to the shader
        glm::mat4 normalMatrix;
        glm::mat4 modelViewMatrix;
        glm::mat4 MVPMatrix;
        glm::mat4 lightSpaceMatrix;

        glm::vec3 up;
        glm::vec3 eyeLocation;
        glm::vec3 eyeDirection;

        glm::vec3 initialUp;
        glm::vec3 initialEyeLocation;
        glm::vec3 initialEyeDirection;
    };

    struct shadowImage
    {
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
        RapidVulkan::Framebuffer framebuffer;
        VkDescriptorImageInfo descriptorImageInfo;
        VkSampler textureSampler;
    };

    // TO DO: Create a base application that creates a swapchain with a Color and Depth attachment
	class ShadowMapping
	{
	private:
		std::vector<std::string> presentationExtensions;
        std::unordered_map<std::string, bool> requestedExtensions;
        std::vector<VkExtensionProperties> availableExtensions;

        RapidVulkan::Instance instance;
        uint32_t suitablePhysicalDeviceIndex;
        VkPhysicalDevice selectedPhysicalDevice;

        RapidVulkan::Device device;
        uint32_t suitableQueueFamilyIndex;
        VkQueue queue;

        VkSurfaceKHR surface;
        VkSurfaceFormatKHR selectedSurfaceFormat;
        VkExtent2D swapchainExtent;
        RapidVulkan::SwapchainKHR swapchain;

        RapidVulkan::RenderPass renderPass;
        std::vector<RapidVulkan::ImageView> swapchainImageViews;
        std::vector<RapidVulkan::Framebuffer> framebuffers;

        std::vector<RapidVulkan::Semaphore> imageAvailable;
        std::vector<RapidVulkan::Semaphore> renderingFinished;

        RapidVulkan::CommandPool graphicsCommandPool;
        RapidVulkan::CommandBuffers graphicsCommandBuffers;
        
        const uint32_t renderResourcesCount;
        std::vector<VkFence> fences;
        
        std::vector<vertexBuffer> vertexBuffers;

        // Final Pass Resources
        renderPassResources finalPass;

        // Depth Pass Resources
        RapidVulkan::RenderPass shadowRenderPass;
        renderPassResources shadowPass;
        std::vector<shadowImage> shadowImages;
        

        void AllocateDescriptorSet();
        void AllocateShadowDescriptorSet();
        void CreateDepthImage();
        void CreateShadowDepthImage();
        void CreateShadowDepthImageSampler();
        void CreateDescriptorPool();
        void CreateShadowDescriptorPool();
        void CreateDescriptorSetLayout();
        void CreateShadowDescriptorSetLayout();
        void CreateFences();
        void CreateGraphicsCommandsBuffers();
        void CreateGraphicsPipeline();
        void CreateShadowPipeline();
        void CreateImageView(const VkImage& image, VkImageView& imageView);
        VkImageView CreateImageViewVulkanTutorial(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        void CreateImageVulkanTutorial(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void CreateInstance();
        void CreateJustInTimeFramebuffer(RapidVulkan::Framebuffer& framebuffer, const RapidVulkan::ImageView& imageView);
        void CreateJustInTimeShadowFramebuffer(RapidVulkan::Framebuffer& framebuffer, const VkImageView& imageView);
        void CreateLogicalDevice();
        void CreateRenderPass();
        void CreateShadowRenderPass();
        void CreateSemaphores();
        void CreateSurface();
        void CreateSwapchain();
        void CreateSwapchainImageViews();
        VkFormat FindDepthFormat();
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void RecordJustInTimeCommandBuffers(const size_t& resourceIndex);
        void RecordJustInTimeShadowCommandBuffers(const size_t& resourceIndex);
        void CreateBuffer(VkBuffer &buffer, VkDeviceMemory& memory, void** mappedMemory, VkBufferUsageFlags usage, VkDeviceSize size, VkMemoryPropertyFlags properties);
        void CreateUniformBuffers();
        void CreateShadowUniformBuffers();
        void UpdateDescriptorSet();
        void UpdateShadowPassDescriptorSet();
	public:
        std::shared_ptr<IWindow> window;
        ShadowMapping();
		~ShadowMapping();
        void Draw();
        void RotateHorizontal(float angle);
        void RotateVertical(float angle);
	};
	

  }