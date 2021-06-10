#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Window.hpp"

// Rubric 5: Memory Management:
// Rapid Vulkan wraps Vulkan Objects and eases RAII implementation
// All RapidVulkan Objects declared in the class, will be automatically destroyed
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

// Sascha Willems's helper files to load glTF models
#include "VulkanDevice.hpp"
#include "camera.hpp"


namespace BadgerSandbox
{
    
    struct DestroyGLFWwindow
    {
        // Rubric 5: Memory Management: I wrapped a GLFWwindow* pointer in a smart pointer to ensure it is destroyed when the class is destroyed.
        void operator()(GLFWwindow* ptr)
        {
            glfwDestroyWindow(ptr);
        }
    };

    // Rubric 4: Object Oriented Programming: Only the appropriate functions are exposed to the class user: 
    // Constructor, destructor, Draw and get window pointer function
    class SandboxApplication
	{
	private:
        // Rubric 5: Memory Management: I wrapped a GLFWwindow* pointer in a smart pointer to ensure it is destroyed when the class is destroyed.
        std::unique_ptr<GLFWwindow, DestroyGLFWwindow> window;
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

        RapidVulkan::GraphicsPipeline graphicsPipeline;

        RapidVulkan::CommandPool graphicsCommandPool;
        RapidVulkan::CommandBuffers graphicsCommandBuffers;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        const size_t renderResourcesCount;
        std::vector<VkFence> fences;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        VkDescriptorSetLayout dsLayout;
        VkDescriptorPool dPool;
        VkDescriptorSet ds;
        VkPipelineLayout pipelineLayout;

        // Sascha's objects needed to load a glTF model:
        vks::VulkanDevice saschaDevice;
        Camera saschaCamera;

        uint32_t currentSelectedModel;

        void AllocateBufferMemory(VkBuffer& buffer, const VkMemoryPropertyFlags& memoryProperty, VkDeviceMemory& memory);
        void AllocateDescriptorSetNode();
        void AllocateDescriptorSetScene();
        void CreateBuffer(const VkDeviceSize& size, VkBufferUsageFlags usage, VkBuffer& bufferToCreate, VkDeviceMemory& memory, const VkMemoryPropertyFlags& memoryProperty);
        void CreateDepthImage();
        void CreateDescriptorPool();
        void CreateDescriptorSetLayoutNode();
        void CreateDescriptorSetLayoutScene();
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
        std::unique_ptr<GLFWwindow, DestroyGLFWwindow> CreateVulkanWindow();
        VkFormat FindDepthFormat();
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void LoadModel1();
        void LoadModel2();
        void LoadModel3();
        void PopulateSaschaWillemsStructures();
        void RecordJustInTimeCommandBuffers(const size_t& resourceIndex);
        void UpdateDescriptorSetScene();

	public:
        SandboxApplication();
		~SandboxApplication();
        void Draw();
        GLFWwindow* GetRawWindow() { return window.get(); }
	};
	

  }