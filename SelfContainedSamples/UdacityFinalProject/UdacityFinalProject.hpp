#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Window.hpp"

// Rapid Vulkan wraps Vulkan Objects and eases RAII implementation
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
  GLFWwindow* window;
  std::vector<std::string> presentationExtensions;
  std::unordered_map<std::string, bool> requestedExtensions;
  std::vector<VkExtensionProperties> availableExtensions;

  RapidVulkan::Instance instance;
  uint32_t suitablePhysicalDeviceIndex = 0xFFFFFFFF;
  VkPhysicalDevice selectedPhysicalDevice;

  RapidVulkan::Device device;
  uint32_t suitableQueueFamilyIndex = 0xFFFFFFFF;
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

  static const size_t renderResourcesCount = 3;
  std::vector<VkFence> fences;

  struct VertexData
  {
	  float   x, y, z, w;
	  float   u, v;
  };

  VkImage textureImage;
  VkImageView textureImageView;
  VkDeviceMemory textureMemory;
  VkSampler textureSampler;

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
}