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

#include "Matrix4D.hpp"
#include "Matrix3D.hpp"

#define GLM_DEPTH_ZERO_TO_ONE   1
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

    // TO DO: Create a base application that creates a swapchain with a Color and Depth attachment
	class VectorTestApplication
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

        RapidVulkan::ShaderModule vertexShaderModule;
        RapidVulkan::ShaderModule fragmentShaderModule;

        VkDescriptorSetLayout descriptorSetLayout;
        std::vector<VkDescriptorSet> descriptorSets;

        RapidVulkan::GraphicsPipeline graphicsPipeline;

        RapidVulkan::CommandPool graphicsCommandPool;
        RapidVulkan::CommandBuffers graphicsCommandBuffers;

        std::vector<vertexBuffer> vertexBuffers;

        const size_t renderResourcesCount;
        std::vector<VkFence> fences;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        VkDescriptorSetLayout dsLayout;
        VkDescriptorPool dPool;
        VkDescriptorSet ds;
        VkPipelineLayout pipelineLayout;

        std::vector<uniformBuffer> matrixUniformBuffers;

        Matrix4D modelMatrix;
        glm::mat4 glmModelMatrix;

        Matrix4D viewMatrix;
        glm::mat4 glmViewMatrix;

        Matrix4D projectionMatrix;
        glm::mat4 glmProjectionMatrix;
        
        Matrix4D modelViewMatrix;
        glm::mat4 glmModelViewMatrix;

        Matrix4D MVPMatrix;
        glm::mat4 glmMVPMatrix;

        Vector3D up;
        Vector3D eyeLocation;
        Vector3D eyeDirection;

        Vector3D initialUp;
        Vector3D initialEyeLocation;
        Vector3D initialEyeDirection;

        void AllocateDescriptorSet();
        void CreateDepthImage();
        void CreateDescriptorPool();
        void CreateDescriptorSetLayout();
        void CreateFences();
        void CreateGraphicsCommandsBuffers();
        void CreateGraphicsPipeline();
        void CreateImageView(const VkImage& image, VkImageView& imageView);
        VkImageView CreateImageViewVulkanTutorial(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        void CreateImageVulkanTutorial(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void CreateInstance();
        void CreateJustInTimeFramebuffer(RapidVulkan::Framebuffer& framebuffer, const RapidVulkan::ImageView& imageView);
        void CreateLogicalDevice();
        void CreateRenderPass();
        void CreateSemaphores();
        void CreateSurface();
        void CreateSwapchain();
        void CreateSwapchainImageViews();
        VkFormat FindDepthFormat();
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void RecordJustInTimeCommandBuffers(const size_t& resourceIndex);
        void CreateBuffer(VkBuffer &buffer, VkDeviceMemory& memory, void** mappedMemory, VkBufferUsageFlags usage, VkDeviceSize size, VkMemoryPropertyFlags properties);
        void CreateUniformBuffers();
        void CreateVertexBuffer();
        void UpdateDescriptorSet();
        Matrix3D Rotate(const float angle, const Vector3D axis);
        Matrix4D LookAt();
        Matrix4D Perspective(float r, float l, float t, float b, float f, float n);
        Matrix4D Perspective(float fov, float aspect, float far, float near);
	public:
        std::shared_ptr<IWindow> window;
        VectorTestApplication();
		~VectorTestApplication();
        void Draw();
        void RotateHorizontal(float angle);
        void RotateVertical(float angle);
	};
	

  }