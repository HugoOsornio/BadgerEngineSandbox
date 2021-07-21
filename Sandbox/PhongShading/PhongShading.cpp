#include <fstream>
#include <iostream>
#include <array>
#include "WindowFactory.hpp"
#include "PhongShading.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanglTFModel.hpp"

namespace BadgerSandbox
{
	vkglTF::Model NyotenguModel;

	void RenderNode(const vkglTF::Node& node, uint32_t cbIndex, VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout)
	{
		if (node.mesh)
		{
			// Render mesh primitives
			for (vkglTF::Primitive* primitive : node.mesh->primitives)
			{
				if (primitive->hasIndices)
				{
					vkCmdDrawIndexed(cmdBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
				}
				else
				{
					vkCmdDraw(cmdBuffer, primitive->vertexCount, 1, 0, 0);
				}
			}

		};
		for (auto child : node.children)
		{
			RenderNode(*child, cbIndex, cmdBuffer, pipelineLayout);
		}
	}

	void PhongShading::CreateInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Badger Sandbox";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t availableExtensionsCount = 0;
		if ((vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr) != VK_SUCCESS) ||
			(availableExtensionsCount == 0))
		{
			throw std::runtime_error("No extensions available");
		}

		availableExtensions.resize(availableExtensionsCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, &availableExtensions[0]) != VK_SUCCESS)
		{
			throw std::runtime_error("Error occurred during instance extensions enumeration!");
		}

		for (uint32_t i = 0; i < availableExtensionsCount; i++)
		{
			std::string currentExtension(availableExtensions[i].extensionName);
			if (requestedExtensions.find(currentExtension) != requestedExtensions.end())
				requestedExtensions[currentExtension] = true;
		}

		std::vector<const char*> confirmedExtensionNames;
		for (const auto& item : requestedExtensions)
		{
			if (!item.second)
			{
				std::string errorMessage("Required Extension " + item.first + " not found");
				throw std::runtime_error(errorMessage.c_str());
			}
			else
			{
				confirmedExtensionNames.push_back(item.first.c_str());
			}
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(confirmedExtensionNames.size());
		createInfo.ppEnabledExtensionNames = confirmedExtensionNames.data();

		const std::vector<const char*> validationLayers =
		{
		  "VK_LAYER_KHRONOS_validation"
		};

		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = nullptr;

		instance.Reset(createInfo);
	}

	void PhongShading::CreateSurface()
	{
		window->GetVulkanSurfaceFromWindow(instance.Get(), &surface);
	}

	void PhongShading::CreateLogicalDevice()
	{
		uint32_t numDevices = 0;
		if ((vkEnumeratePhysicalDevices(instance.Get(), &numDevices, nullptr) != VK_SUCCESS) ||
			(numDevices == 0))
		{
			throw std::runtime_error("Couldn't get a Physical Device");
		}

		std::vector<VkPhysicalDevice> physicalDevices(numDevices);
		if (vkEnumeratePhysicalDevices(instance.Get(), &numDevices, physicalDevices.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Error occurred during physical devices enumeration!");
		}

		std::vector<VkPhysicalDeviceProperties> deviceProperties{};
		std::vector<VkPhysicalDeviceFeatures> deviceFeatures{};
		deviceProperties.resize(physicalDevices.size());
		deviceFeatures.resize(physicalDevices.size());
		for (uint32_t i = 0; i < physicalDevices.size(); i++)
		{
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties[i]);
			vkGetPhysicalDeviceFeatures(physicalDevices[i], &deviceFeatures[i]);
		}

		for (uint32_t i = 0; i < physicalDevices.size(); i++)
		{
			uint32_t queueFamiliesCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamiliesCount, nullptr);
			if (queueFamiliesCount == 0)
			{
				std::cout << "Physical device " << physicalDevices[i] << " doesn't have any queue families!" << std::endl;
				continue;
			}

			std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamiliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamiliesCount, &queueFamilyProperties[0]);
			for (uint32_t j = 0; j < queueFamiliesCount; ++j)
			{
				VkBool32 queuePresentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, surface, &queuePresentSupport);

				if ((queueFamilyProperties[i].queueCount > 0) &&
					(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					queuePresentSupport)
				{
					suitablePhysicalDeviceIndex = i;
					suitableQueueFamilyIndex = j;
					break;
				}
			}
		}

		if (suitablePhysicalDeviceIndex == 0xFFFFFFFF || suitableQueueFamilyIndex == 0xFFFFFFFF)
		{
			throw std::runtime_error("Didn't find a physical device with queues that support graphics and presentation");
		}

		selectedPhysicalDevice = physicalDevices[suitablePhysicalDeviceIndex];

		std::vector<float> queuePriorities = { 1.0f };

		VkDeviceQueueCreateInfo queueCreateInfo = {
		  VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,     // VkStructureType              sType
		  nullptr,                                        // const void                  *pNext
		  0,                                              // VkDeviceQueueCreateFlags     flags
		  suitableQueueFamilyIndex,                       // uint32_t                     queueFamilyIndex
		  static_cast<uint32_t>(queuePriorities.size()),  // uint32_t                     queueCount
		  &queuePriorities[0]                             // const float                 *pQueuePriorities
		};

		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.fillModeNonSolid = deviceFeatures[suitablePhysicalDeviceIndex].fillModeNonSolid;
		enabledFeatures.wideLines = deviceFeatures[suitablePhysicalDeviceIndex].wideLines;

		std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo deviceCreateInfo = {
		  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,           // VkStructureType                    sType
		  nullptr,                                        // const void                        *pNext
		  0,                                              // VkDeviceCreateFlags                flags
		  1,                                              // uint32_t                           queueCreateInfoCount
		  &queueCreateInfo,                               // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
		  0,                                              // uint32_t                           enabledLayerCount
		  nullptr,                                        // const char * const                *ppEnabledLayerNames
		  deviceExtensions.size(),                        // uint32_t                           enabledExtensionCount
		  deviceExtensions.data(),                        // const char * const                *ppEnabledExtensionNames
		  &enabledFeatures                                         // const VkPhysicalDeviceFeatures    *pEnabledFeatures
		};

		device.Reset(selectedPhysicalDevice, deviceCreateInfo);

		vkGetDeviceQueue(device.Get(), suitableQueueFamilyIndex, 0, &queue);
	}

	void PhongShading::CreateSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo =
		{
		  VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,      // VkStructureType          sType
		  nullptr,                                      // const void*              pNext
		  0                                             // VkSemaphoreCreateFlags   flags
		};

		imageAvailable.resize(renderResourcesCount);
		renderingFinished.resize(renderResourcesCount);

		for (size_t i = 0; i < renderResourcesCount; i++)
		{
			imageAvailable[i].Reset(device.Get(), semaphoreCreateInfo);
			renderingFinished[i].Reset(device.Get(), semaphoreCreateInfo);
		}
	}

	void PhongShading::CreateFences()
	{
		VkFenceCreateInfo fenceCreateInfo =
		{
		  VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,              // VkStructureType                sType
		  nullptr,                                          // const void                    *pNext
		  VK_FENCE_CREATE_SIGNALED_BIT                      // VkFenceCreateFlags             flags
		};

		fences.resize(renderResourcesCount);

		for (size_t i = 0; i < renderResourcesCount; ++i)
		{
			if (vkCreateFence(device.Get(), &fenceCreateInfo, nullptr, &fences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Couldn't create a fence");
			}
		}
	}

	void PhongShading::CreateSwapchain()
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(selectedPhysicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not check presentation surface capabilities");
		}

		uint32_t formatsCount;
		if ((vkGetPhysicalDeviceSurfaceFormatsKHR(selectedPhysicalDevice, surface, &formatsCount, nullptr) != VK_SUCCESS) ||
			(formatsCount == 0))
		{
			throw std::runtime_error("Couldn't get the number Device Surface Formats");
		}

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(selectedPhysicalDevice, surface, &formatsCount, surfaceFormats.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Couldn't get the Device Surface Formats");
		}

		uint32_t presentModesCount;
		if ((vkGetPhysicalDeviceSurfacePresentModesKHR(selectedPhysicalDevice, surface, &presentModesCount, nullptr) != VK_SUCCESS) ||
			(presentModesCount == 0))
		{
			throw std::runtime_error("Couldn't get the Presentation Modes count");
		}

		std::vector<VkPresentModeKHR> presentModes(presentModesCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(selectedPhysicalDevice, surface, &presentModesCount, presentModes.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Couldn't get the Presentation Modes count");
		}

		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		if ((surfaceCapabilities.maxImageCount > 0) &&
			(imageCount > surfaceCapabilities.maxImageCount))
		{
			imageCount = surfaceCapabilities.maxImageCount;
		}

		// Check if list contains most widely used R8 G8 B8 A8 format
		// with nonlinear color space
		for (VkSurfaceFormatKHR& sf : surfaceFormats)
		{
			if (sf.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				selectedSurfaceFormat = sf;
				break;
			}
		}
		if (selectedSurfaceFormat.format != VK_FORMAT_B8G8R8A8_SRGB)
		{
			std::cout << "Found an undefined format, forcing BGRA8888 UNORM";
			selectedSurfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
			selectedSurfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		}

		// Special value of surface extent is width == height == 0xFFFFFFFF
		// If this is so we define the size by ourselves but it must fit within defined confines
		if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
		{
			std::array<uint32_t, 2> windowSize { 0, 0};
			windowSize = window->GetWindowSize();
			swapchainExtent.width = windowSize[0];
			swapchainExtent.height = windowSize[1];
			if (swapchainExtent.width < surfaceCapabilities.minImageExtent.width) {
				swapchainExtent.width = surfaceCapabilities.minImageExtent.width;
			}
			if (swapchainExtent.height < surfaceCapabilities.minImageExtent.height) {
				swapchainExtent.height = surfaceCapabilities.minImageExtent.height;
			}
			if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.width) {
				swapchainExtent.width = surfaceCapabilities.maxImageExtent.width;
			}
			if (swapchainExtent.height > surfaceCapabilities.maxImageExtent.height) {
				swapchainExtent.height = surfaceCapabilities.maxImageExtent.height;
			}
		}
		else
		{
			swapchainExtent = surfaceCapabilities.currentExtent;
		}

		// Color attachment flag must always be supported
		// We can define other usage flags but we always need to check if they are supported
		VkImageUsageFlags supportedUsageFlags{};
		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		{
			supportedUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		// Sometimes images must be transformed before they are presented (i.e. due to device's orienation
		// being other than default orientation)
		// If the specified transform is other than current transform, presentation engine will transform image
		// during presentation operation; this operation may hit performance on some platforms
		// Here we don't want any transformations to occur so if the identity transform is supported use it
		// otherwise just use the same transform as current transform
		VkSurfaceTransformFlagBitsKHR surfaceTransform{};
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			surfaceTransform = surfaceCapabilities.currentTransform;
		}

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (VkPresentModeKHR& pm : presentModes)
		{
			if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = pm;
			}
		}
		if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			std::cout << "MAILBOX present mode is not available, forcing FIFO as present mode" << std::endl;
			presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}

		VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE;

		VkSwapchainCreateInfoKHR swapchainCreateInfo =
		{
		  VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // VkStructureType                sType
		  nullptr,                                      // const void                    *pNext
		  0,                                            // VkSwapchainCreateFlagsKHR      flags
		  surface,                                      // VkSurfaceKHR                   surface
		  imageCount,                                   // uint32_t                       minImageCount
		  selectedSurfaceFormat.format,                 // VkFormat                       imageFormat
		  selectedSurfaceFormat.colorSpace,             // VkColorSpaceKHR                imageColorSpace
		  swapchainExtent,                              // VkExtent2D                     imageExtent
		  1,                                            // uint32_t                       imageArrayLayers
		  supportedUsageFlags,                          // VkImageUsageFlags              imageUsage
		  VK_SHARING_MODE_EXCLUSIVE,                    // VkSharingMode                  imageSharingMode
		  0,                                            // uint32_t                       queueFamilyIndexCount
		  nullptr,                                      // const uint32_t                *pQueueFamilyIndices
		  surfaceTransform,                             // VkSurfaceTransformFlagBitsKHR  preTransform
		  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // VkCompositeAlphaFlagBitsKHR    compositeAlpha
		  presentMode,                                  // VkPresentModeKHR               presentMode
		  VK_TRUE,                                      // VkBool32                       clipped
		  oldSwapChain                                  // VkSwapchainKHR                 oldSwapchain
		};

		swapchain.Reset(device.Get(), swapchainCreateInfo);

		if (oldSwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device.Get(), oldSwapChain, nullptr);
		}
	}

	void PhongShading::CreateRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachmentDescriptions
		{ {
		 {
			0,                                   // VkAttachmentDescriptionFlags   flags
			selectedSurfaceFormat.format,        // VkFormat                       format
			VK_SAMPLE_COUNT_1_BIT,               // VkSampleCountFlagBits          samples
			VK_ATTACHMENT_LOAD_OP_CLEAR,         // VkAttachmentLoadOp             loadOp
			VK_ATTACHMENT_STORE_OP_STORE,        // VkAttachmentStoreOp            storeOp
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,     // VkAttachmentLoadOp             stencilLoadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE,    // VkAttachmentStoreOp            stencilStoreOp
			VK_IMAGE_LAYOUT_UNDEFINED,           // VkImageLayout                  initialLayout;
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR      // VkImageLayout                  finalLayout
		 },
		 {
		   0,                                   // VkAttachmentDescriptionFlags   flags
		   FindDepthFormat(),                   // VkFormat                       format
		   VK_SAMPLE_COUNT_1_BIT,               // VkSampleCountFlagBits          samples
		   VK_ATTACHMENT_LOAD_OP_CLEAR,         // VkAttachmentLoadOp             loadOp
		   VK_ATTACHMENT_STORE_OP_DONT_CARE,    // VkAttachmentStoreOp            storeOp
		   VK_ATTACHMENT_LOAD_OP_DONT_CARE,     // VkAttachmentLoadOp             stencilLoadOp
		   VK_ATTACHMENT_STORE_OP_DONT_CARE,    // VkAttachmentStoreOp            stencilStoreOp
		   VK_IMAGE_LAYOUT_UNDEFINED,           // VkImageLayout                  initialLayout;
		   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL      // VkImageLayout                  finalLayout
		 }
		} };

		std::array<VkAttachmentReference, 2> attachmentReferences
		{ {
		  {
			0,                                          // uint32_t                       attachment
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // VkImageLayout                  layout
		  },
		  {
			1,                                                 // uint32_t                       attachment
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL   // VkImageLayout                  layout
		  }
		} };

		std::array<VkSubpassDescription, 1> subpassesDescriptions
		{
		  {
			0,                                          // VkSubpassDescriptionFlags      flags
			VK_PIPELINE_BIND_POINT_GRAPHICS,            // VkPipelineBindPoint            pipelineBindPoint
			0,                                          // uint32_t                       inputAttachmentCount
			nullptr,                                    // const VkAttachmentReference   *pInputAttachments
			1,                                          // uint32_t                       colorAttachmentCount
			&attachmentReferences[0],                   // const VkAttachmentReference   *pColorAttachments
			nullptr,                                    // const VkAttachmentReference   *pResolveAttachments
			&attachmentReferences[1],                   // const VkAttachmentReference   *pDepthStencilAttachment
			0,                                          // uint32_t                       preserveAttachmentCount
			nullptr                                     // const uint32_t*                pPreserveAttachments
		  }
		};

		std::vector<VkSubpassDependency> dependencies =
		{
		  {
			VK_SUBPASS_EXTERNAL,                            // uint32_t                       srcSubpass
			0,                                              // uint32_t                       dstSubpass
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,           // VkPipelineStageFlags           srcStageMask
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,           // VkPipelineStageFlags           dstStageMask
			0,                      // VkAccessFlags                  srcAccessMask
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,                   // VkAccessFlags                  dstAccessMask
			VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags
		  },
		};

		VkRenderPassCreateInfo renderPassCreateInfo =
		{
		  VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,    // VkStructureType                sType
		  nullptr,                                      // const void                    *pNext                                           
		  0,                                            // VkRenderPassCreateFlags        flags
		  attachmentDescriptions.size(),                // uint32_t                       attachmentCount
		  attachmentDescriptions.data(),                // const VkAttachmentDescription *pAttachments
		  subpassesDescriptions.size(),                 // uint32_t                       subpassCount
		  subpassesDescriptions.data(),                 // const VkSubpassDescription    *pSubpasses
		  dependencies.size(),                          // uint32_t                       dependencyCount
		  dependencies.data()                           // const VkSubpassDependency     *pDependencies
		};

		renderPass.Reset(device.Get(), renderPassCreateInfo);
	}

	void PhongShading::CreateSwapchainImageViews()
	{
		uint32_t numberOfSwapchainImages = 0;
		swapchain.GetSwapchainImagesKHR(&numberOfSwapchainImages, nullptr);
		std::vector<VkImage> swapchainImages(numberOfSwapchainImages);
		swapchain.GetSwapchainImagesKHR(&numberOfSwapchainImages, swapchainImages.data());
		swapchainImageViews.resize(numberOfSwapchainImages);

		for (uint32_t i = 0; i < numberOfSwapchainImages; i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // VkStructureType                sType
				nullptr,                                    // const void                    *pNext
				0,                                          // VkImageViewCreateFlags         flags
				swapchainImages[i],                         // VkImage                        image
				VK_IMAGE_VIEW_TYPE_2D,                      // VkImageViewType                viewType
				selectedSurfaceFormat.format,               // VkFormat                       format
				{                                           // VkComponentMapping             components
				  VK_COMPONENT_SWIZZLE_IDENTITY,              // VkComponentSwizzle             r
				  VK_COMPONENT_SWIZZLE_IDENTITY,              // VkComponentSwizzle             g
				  VK_COMPONENT_SWIZZLE_IDENTITY,              // VkComponentSwizzle             b
				  VK_COMPONENT_SWIZZLE_IDENTITY               // VkComponentSwizzle             a
				},
				{                                           // VkImageSubresourceRange        subresourceRange
				  VK_IMAGE_ASPECT_COLOR_BIT,                  // VkImageAspectFlags             aspectMask
				  0,                                          // uint32_t                       baseMipLevel
				  1,                                          // uint32_t                       levelCount
				  0,                                          // uint32_t                       baseArrayLayer
				  1                                           // uint32_t                       layerCount
				}
			};
			swapchainImageViews[i].Reset(device.Get(), imageViewCreateInfo);
		}
	}

	/*
  Took this function from VulkanTutorial:
  https://vulkan-tutorial.com/
*/
	uint32_t PhongShading::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(selectedPhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	/*
	  Took this function from VulkanTutorial:
	  https://vulkan-tutorial.com/
	*/
	void PhongShading::CreateImageVulkanTutorial(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device.Get(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device.Get(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device.Get(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device.Get(), image, imageMemory, 0);
	}

	/*
	  Took this function from VulkanTutorial:
	  https://vulkan-tutorial.com/
	*/
	VkImageView PhongShading::CreateImageViewVulkanTutorial(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device.Get(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture image view!");
		}
		return imageView;
	}

	/*
	  Took this function from VulkanTutorial:
	  https://vulkan-tutorial.com/
	*/
	VkFormat PhongShading::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(selectedPhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	/*
	  Took this function from VulkanTutorial:
	  https://vulkan-tutorial.com/
	*/
	VkFormat PhongShading::FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	/*
	  Took this function from VulkanTutorial:
	  https://vulkan-tutorial.com/
	*/
	void PhongShading::CreateDepthImage()
	{
		VkFormat depthFormat = FindDepthFormat();
		CreateImageVulkanTutorial(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = CreateImageViewVulkanTutorial(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void PhongShading::CreateGraphicsCommandsBuffers()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo =
		{
		  VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,           // VkStructureType                sType
		  nullptr,                                              // const void                    *pNext
		  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
		  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,                 // VkCommandPoolCreateFlags       flags
		  suitableQueueFamilyIndex                              // uint32_t                       queueFamilyIndex
		};

		graphicsCommandPool.Reset(device.Get(), commandPoolCreateInfo);

		VkCommandBufferAllocateInfo commandBufferAllocateInfo =
		{
		  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType                sType
		  nullptr,                                        // const void                    *pNext
		  graphicsCommandPool.Get(),                      // VkCommandPool                  commandPool
		  VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel           level
		  renderResourcesCount                            // uint32_t                       bufferCount
		};

		graphicsCommandBuffers.Reset(device.Get(), commandBufferAllocateInfo);

		VkCommandBufferBeginInfo graphicsCommandBufferBeginInfo =
		{
		  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType                        sType
		  nullptr,                                        // const void                            *pNext
		  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,   // VkCommandBufferUsageFlags              flags
		  nullptr                                         // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
		};

		VkImageSubresourceRange imageSubresourceRange = {
		  VK_IMAGE_ASPECT_COLOR_BIT,                      // VkImageAspectFlags             aspectMask
		  0,                                              // uint32_t                       baseMipLevel
		  1,                                              // uint32_t                       levelCount
		  0,                                              // uint32_t                       baseArrayLayer
		  1                                               // uint32_t                       layerCount
		};

		VkClearValue clearValue = {
		  { 1.0f, 0.8f, 0.4f, 0.0f },                     // VkClearColorValue              color
		};

		framebuffers.resize(renderResourcesCount);
	}

	void PhongShading::CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding layoutBinding =
		{
			0,                                                    // uint32_t                             binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                    // VkDescriptorType                     descriptorType
			1,                                                    // uint32_t                             descriptorCount
			VK_SHADER_STAGE_VERTEX_BIT,                         // VkShaderStageFlags                   stageFlags
			nullptr                                               // const VkSampler                     *pImmutableSamplers
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
		{
		  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,  // VkStructureType                      sType
		  nullptr,                                              // const void                          *pNext
		  0,                                                    // VkDescriptorSetLayoutCreateFlags     flags
		  1,                                                    // uint32_t                             bindingCount
		  &layoutBinding                                       // const VkDescriptorSetLayoutBinding  *pBindings
		};

		RapidVulkan::CheckError(vkCreateDescriptorSetLayout(device.Get(), &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
	}

	void PhongShading::CreateDescriptorPool()
	{
		uint32_t meshCount = 0;
		VkDescriptorPoolSize poolSizes = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, renderResourcesCount};
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.poolSizeCount = 1;
		descriptorPoolCI.pPoolSizes = &poolSizes;
		descriptorPoolCI.maxSets = renderResourcesCount;

		RapidVulkan::CheckError(vkCreateDescriptorPool(device.Get(), &descriptorPoolCI, nullptr, &dPool));
	}

	void PhongShading::AllocateDescriptorSet()
	{
		descriptorSets.resize(renderResourcesCount);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
		{
		  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // VkStructureType                sType
		  nullptr,                                        // const void                    *pNext
		  dPool,                                          // VkDescriptorPool               descriptorPool
		  1,                                              // uint32_t                       descriptorSetCount
		  & descriptorSetLayout                           // const VkDescriptorSetLayout   *pSetLayouts
		};
		for (uint32_t i = 0; i < renderResourcesCount; i++)
		{
			RapidVulkan::CheckError(vkAllocateDescriptorSets(device.Get(), &descriptorSetAllocateInfo, &descriptorSets[i]));
		}
	}

	void PhongShading::CreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory, void** mappedMemory, VkBufferUsageFlags usage, VkDeviceSize size, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.usage = usage;
		createInfo.size = size;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		vkCreateBuffer(device.Get(), &createInfo, nullptr, &buffer);

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device.Get(), buffer, &memoryRequirements);

		VkMemoryAllocateInfo allocationInfo{};
		allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocationInfo.allocationSize = memoryRequirements.size;
		allocationInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device.Get(), &allocationInfo, nullptr, &memory) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device.Get(), buffer, memory, 0);

     	// Map the buffer so we can write on it as needed.
		vkMapMemory(device.Get(), memory, 0, allocationInfo.allocationSize, 0, mappedMemory);
	}

	void PhongShading::CreateUniformBuffers()
	{
	   // Matrix uniform buffers will contain:
	   // A modelview matrix and the MVP matrix, each of them will have 16 floats.
		matrixUniformBuffers.resize(renderResourcesCount);
		size_t bufferSize = sizeof(glm::mat4) * 2;
		for (auto& uBuffer : matrixUniformBuffers)
		{
			CreateBuffer(uBuffer.buffer, uBuffer.memory, &uBuffer.mapped, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uBuffer.decriptor.buffer = uBuffer.buffer;
			uBuffer.decriptor.offset = 0;
			uBuffer.decriptor.range = bufferSize;
		}
	}

	void PhongShading::UpdateDescriptorSet()
	{
		for (uint32_t i = 0; i < renderResourcesCount; i++)
		{
			VkWriteDescriptorSet writeDescriptorSet = {};

			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.dstSet = descriptorSets[i];
			writeDescriptorSet.dstBinding = 0;
			writeDescriptorSet.pBufferInfo = &(matrixUniformBuffers[i].decriptor);

			vkUpdateDescriptorSets(device.Get(), 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	void PhongShading::CreateVertexBuffer()
	{
		float vertexData[6][8] =
		{
			{0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0},
			{0.5, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0},
			
			{0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0},
			{0.0, 0.5, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0},
			
			{0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0},
			{0.0, 0.0, 0.5, 1.0, 0.0, 0.0, 1.0, 1.0}
		};
		
		vertexBuffers.resize(renderResourcesCount);
		for (auto& vBuffer : vertexBuffers)
		{
			CreateBuffer(vBuffer.buffer, vBuffer.memory, &vBuffer.mapped, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vertexData), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			std::memcpy(vBuffer.mapped, vertexData, sizeof(vertexData));
			VkMappedMemoryRange flushRange =
			{
			  VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,            // VkStructureType        sType
			  nullptr,                                          // const void            *pNext
			  vBuffer.memory,                                   // VkDeviceMemory         memory
			  0,                                                // VkDeviceSize           offset
			  VK_WHOLE_SIZE                                     // VkDeviceSize           size
			};
			vkFlushMappedMemoryRanges(device.Get(), 1, &flushRange);
			vkUnmapMemory(device.Get(), vBuffer.memory);
		}
	}

	void PhongShading::CreateGraphicsPipeline()
	{
		std::string vertexShaderPath(std::string(PHONG_PROJECT_CONTENT) + "vert.spv");
		std::ifstream vertexShaderIs(vertexShaderPath, std::ios::binary);

		if (vertexShaderIs.fail())
		{
			throw std::runtime_error("COULD NOT OPEN FILE");
		}
		std::streampos begin, end;
		begin = vertexShaderIs.tellg();
		vertexShaderIs.seekg(0, std::ios::end);
		end = vertexShaderIs.tellg();
		std::vector<char> vertexShaderBuffer(static_cast<size_t>(end - begin));
		vertexShaderIs.seekg(0, std::ios::beg);
		vertexShaderIs.read(&vertexShaderBuffer[0], end - begin);
		vertexShaderIs.close();

		VkShaderModuleCreateInfo shaderModuleCreateInfo;
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.pNext = nullptr;
		shaderModuleCreateInfo.flags = 0;
		shaderModuleCreateInfo.codeSize = vertexShaderBuffer.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderBuffer.data());

		vertexShaderModule.Reset(device.Get(), shaderModuleCreateInfo);

		// Rubric 3: The program reads data from a file
		std::string fragmentShaderPath(std::string(PHONG_PROJECT_CONTENT) + "frag.spv");
		std::ifstream fragmentShaderIs(fragmentShaderPath, std::ios::binary);

		if (fragmentShaderIs.fail())
		{
			throw std::runtime_error("COULD NOT OPEN FILE");
		}

		begin = fragmentShaderIs.tellg();
		fragmentShaderIs.seekg(0, std::ios::end);
		end = fragmentShaderIs.tellg();
		std::vector<char> fragmentShaderBuffer(static_cast<size_t>(end - begin));
		fragmentShaderIs.seekg(0, std::ios::beg);
		fragmentShaderIs.read(&fragmentShaderBuffer[0], end - begin);
		fragmentShaderIs.close();

		shaderModuleCreateInfo.codeSize = fragmentShaderBuffer.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBuffer.data());

		fragmentShaderModule.Reset(device.Get(), shaderModuleCreateInfo);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos =
		{
		  {
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
			nullptr,                                                    // const void                                    *pNext
			0,                                                          // VkPipelineShaderStageCreateFlags               flags
			VK_SHADER_STAGE_VERTEX_BIT,                                 // VkShaderStageFlagBits                          stage
			vertexShaderModule.Get(),                                   // VkShaderModule                                 module
			"main",                                                     // const char                                    *pName
			nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo  // Fragment shader
		  },
		  {
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
			nullptr,                                                    // const void                                    *pNext
			0,                                                          // VkPipelineShaderStageCreateFlags               flags
			VK_SHADER_STAGE_FRAGMENT_BIT,                               // VkShaderStageFlagBits                          stage
			fragmentShaderModule.Get(),                                 // VkShaderModule                                 module
			"main",                                                     // const char                                    *pName
			nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo
		  }
		};

		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions =
		{
		  {
			0,                                                          // uint32_t                                       binding
			sizeof(vkglTF::Model::Vertex),                              // uint32_t                                       stride
			VK_VERTEX_INPUT_RATE_VERTEX                                 // VkVertexInputRate                              inputRate
		  }
		};

		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions =
		{
		  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
		  {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3},
		  {2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6},
		  {3, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 8},
		  {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 10},
		  {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 14}
		};

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,      // VkStructureType                                sType
		  nullptr,                                                        // const void                                    *pNext
		  0,                                                              // VkPipelineVertexInputStateCreateFlags          flags;
		  vertexInputBindingDescriptions.size(),                          // uint32_t                                       vertexBindingDescriptionCount
		  vertexInputBindingDescriptions.data(),                          // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
		  vertexAttributeDescriptions.size(),                             // uint32_t                                       vertexAttributeDescriptionCount
		  vertexAttributeDescriptions.data()                              // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
		};


		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineInputAssemblyStateCreateFlags        flags
		  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                          // VkPrimitiveTopology                            topology
		  VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
		};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,        // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineViewportStateCreateFlags             flags
		  1,                                                            // uint32_t                                       viewportCount
		  nullptr,                                                      // const VkViewport                              *pViewports
		  1,                                                            // uint32_t                                       scissorCount
		  nullptr                                                       // const VkRect2D                                *pScissors
		};

		std::array<VkDynamicState, 2> dynamicStates =
		{
		  VK_DYNAMIC_STATE_VIEWPORT,
		  VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,         // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineDynamicStateCreateFlags              flags
		  static_cast<uint32_t>(dynamicStates.size()),                  // uint32_t                                       dynamicStateCount
		  dynamicStates.data()                                          // const VkDynamicState                          *pDynamicStates
		};

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,   // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineRasterizationStateCreateFlags        flags
		  VK_FALSE,                                                     // VkBool32                                       depthClampEnable
		  VK_FALSE,                                                     // VkBool32                                       rasterizerDiscardEnable
		  VK_POLYGON_MODE_FILL,                                         // VkPolygonMode                                  polygonMode
		  VK_CULL_MODE_BACK_BIT,                                            // VkCullModeFlags                                cullMode
		  VK_FRONT_FACE_COUNTER_CLOCKWISE,                              // VkFrontFace                                    frontFace
		  VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
		  0.0f,                                                         // float                                          depthBiasConstantFactor
		  0.0f,                                                         // float                                          depthBiasClamp
		  0.0f,                                                         // float                                          depthBiasSlopeFactor
		  2.0f                                                          // float                                          lineWidth
		};

		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,     // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineMultisampleStateCreateFlags          flags
		  VK_SAMPLE_COUNT_1_BIT,                                        // VkSampleCountFlagBits                          rasterizationSamples
		  VK_FALSE,                                                     // VkBool32                                       sampleShadingEnable
		  1.0f,                                                         // float                                          minSampleShading
		  nullptr,                                                      // const VkSampleMask                            *pSampleMask
		  VK_FALSE,                                                     // VkBool32                                       alphaToCoverageEnable
		  VK_FALSE                                                      // VkBool32                                       alphaToOneEnable
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_FALSE,
			VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
		{
		  VK_FALSE,                                                     // VkBool32                                       blendEnable
		  VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcColorBlendFactor
		  VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstColorBlendFactor
		  VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
		  VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
		  VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstAlphaBlendFactor
		  VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
		  VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
		  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,     // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineColorBlendStateCreateFlags           flags
		  VK_FALSE,                                                     // VkBool32                                       logicOpEnable
		  VK_LOGIC_OP_COPY,                                             // VkLogicOp                                      logicOp
		  1,                                                            // uint32_t                                       attachmentCount
		  &colorBlendAttachmentState,                                  // const VkPipelineColorBlendAttachmentState     *pAttachments
		  { 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
		};

		// Pipeline layout
		const std::vector<VkDescriptorSetLayout> setLayouts = { descriptorSetLayout };

		VkPipelineLayoutCreateInfo layoutCreateInfo =
		{
		  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // VkStructureType                sType
		  nullptr,                                        // const void                    *pNext
		  0,                                              // VkPipelineLayoutCreateFlags    flags
		  setLayouts.size(),                              // uint32_t                       setLayoutCount
		  setLayouts.data(),                              // const VkDescriptorSetLayout   *pSetLayouts
		  0,                                              // uint32_t                       pushConstantRangeCount
		  nullptr                                         // const VkPushConstantRange     *pPushConstantRanges
		};

		if (vkCreatePipelineLayout(device.Get(), &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			std::cout << "Couldn't create a Pipeline Layout" << std::endl;
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		{
		  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,              // VkStructureType                                sType
		  nullptr,                                                      // const void                                    *pNext
		  0,                                                            // VkPipelineCreateFlags                          flags
		  static_cast<uint32_t>(shaderStageCreateInfos.size()),         // uint32_t                                       stageCount
		  shaderStageCreateInfos.data(),                                // const VkPipelineShaderStageCreateInfo         *pStages
		  &vertexInputStateCreateInfo,                                  // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
		  &inputAssemblyStateCreateInfo,                                // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
		  nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
		  &viewportStateCreateInfo,                                     // const VkPipelineViewportStateCreateInfo       *pViewportState
		  &rasterizationStateCreateInfo,                                // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
		  &multisampleStateCreateInfo,                                  // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
		  &depthStencilCreateInfo,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
		  &colorBlendStateCreateInfo,                                   // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
		  &dynamicStateCreateInfo,                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
		  pipelineLayout,                                               // VkPipelineLayout                               layout
		  renderPass.Get(),                                             // VkRenderPass                                   renderPass
		  0,                                                            // uint32_t                                       subpass
		  VK_NULL_HANDLE,                                               // VkPipeline                                     basePipelineHandle
		  -1                                                            // int32_t                                        basePipelineIndex
		};
		VkPipelineCache newCache = VK_NULL_HANDLE;
		graphicsPipeline.Reset(device.Get(), newCache, pipelineCreateInfo);
	}

	void PhongShading::CreateJustInTimeFramebuffer(RapidVulkan::Framebuffer& framebuffer, const RapidVulkan::ImageView& imageView)
	{
		std::array<VkImageView, 2> attachments =
		{
				imageView.Get(),
				depthImageView
		};
		VkFramebufferCreateInfo framebufferCreateInfo =
		{
		  VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,      // VkStructureType                sType
		  nullptr,                                        // const void                    *pNext
		  0,                                              // VkFramebufferCreateFlags       flags
		  renderPass.Get(),                               // VkRenderPass                   renderPass
		  attachments.size(),                             // uint32_t                       attachmentCount
		  attachments.data(),                             // const VkImageView             *pAttachments
		  swapchainExtent.width,                          // uint32_t                       width
		  swapchainExtent.height,                         // uint32_t                       height
		  1                                               // uint32_t                       layers
		};

		framebuffer.Reset(device.Get(), framebufferCreateInfo);
	}

	void PhongShading::RecordJustInTimeCommandBuffers(const size_t& resourceIndex)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo =
		{
		  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        // VkStructureType                        sType
		  nullptr,                                            // const void                            *pNext
		  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        // VkCommandBufferUsageFlags              flags
		  nullptr                                             // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
		};
		std::vector<VkCommandBuffer> commandBuffers = graphicsCommandBuffers.Get();
		vkBeginCommandBuffer(commandBuffers[resourceIndex], &commandBufferBeginInfo);

		VkImageSubresourceRange image_subresource_range =
		{
		  VK_IMAGE_ASPECT_COLOR_BIT,                          // VkImageAspectFlags                     aspectMask
		  0,                                                  // uint32_t                               baseMipLevel
		  1,                                                  // uint32_t                               levelCount
		  0,                                                  // uint32_t                               baseArrayLayer
		  1                                                   // uint32_t                               layerCount
		};

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo =
		{
		  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,           // VkStructureType                        sType
		  nullptr,                                            // const void                            *pNext
		  renderPass.Get(),                                   // VkRenderPass                           renderPass
		  framebuffers[resourceIndex].Get(),                  // VkFramebuffer                          framebuffer
		  {                                                   // VkRect2D                               renderArea
			{                                                 // VkOffset2D                             offset
			  0,                                              // int32_t                                x
			  0                                               // int32_t                                y
			},
			swapchainExtent,                                  // VkExtent2D                             extent;
		  },
		  clearValues.size(),                                                  // uint32_t                               clearValueCount
		  clearValues.data()                                         // const VkClearValue                    *pClearValues
		};

		// Update UBOs
		modelMatrix = glm::mat4(1.0f);
		viewMatrix = glm::lookAt(eyeLocation, eyeDirection, up);
		projectionMatrix = glm::perspective(glm::radians(70.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		projectionMatrix[1][1] *= -1;

		modelViewMatrix = viewMatrix * modelMatrix;
		MVPMatrix = projectionMatrix * viewMatrix * modelMatrix;
		
		uniformBuffer currentUniformBuffer = matrixUniformBuffers[resourceIndex];
		float* memory = (float*)currentUniformBuffer.mapped;
		memcpy((void*)memory, glm::value_ptr(modelViewMatrix) , sizeof(glm::mat4));
		memory += sizeof(glm::mat4)/sizeof(float);
		memcpy((void*)memory, glm::value_ptr(MVPMatrix), sizeof(glm::mat4));

		vkCmdBeginRenderPass(commandBuffers[resourceIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[resourceIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.Get());

		VkViewport viewport =
		{
		  0.0f,                                               // float                                  x
		  0.0f,                                               // float                                  y
		  static_cast<float>(swapchainExtent.width),          // float                                  width
		  static_cast<float>(swapchainExtent.height),         // float                                  height
		  0.0f,                                               // float                                  minDepth
		  1.0f                                                // float                                  maxDepth
		};

		VkRect2D scissor = {
		  {                                                   // VkOffset2D                             offset
			0,                                                  // int32_t                                x
			0                                                   // int32_t                                y
		  },
		  {                                                   // VkExtent2D                             extent
			swapchainExtent.width,                              // uint32_t                               width
			swapchainExtent.height                             // uint32_t                               height
		  }
		};

		vkCmdSetViewport(commandBuffers[resourceIndex], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[resourceIndex], 0, 1, &scissor);
		
		VkDeviceSize offset = 0;
		vkCmdBindDescriptorSets(commandBuffers[resourceIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
			&(descriptorSets[resourceIndex]), 0, nullptr);
		vkCmdBindVertexBuffers(commandBuffers[resourceIndex], 0, 1, &NyotenguModel.vertices.buffer, &offset);
		if (NyotenguModel.indices.buffer != VK_NULL_HANDLE)
		{
			vkCmdBindIndexBuffer(commandBuffers[resourceIndex], NyotenguModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		}
		for (auto node : NyotenguModel.nodes)
		{
			RenderNode(*node, resourceIndex, commandBuffers[resourceIndex], pipelineLayout);
		}
		
		

		vkCmdEndRenderPass(commandBuffers[resourceIndex]);

		if (vkEndCommandBuffer(commandBuffers[resourceIndex]) != VK_SUCCESS)
		{
			std::cout << "Could not record command buffer!" << std::endl;
		}
	}

	void PhongShading::Draw()
	{
		static size_t resourceIndex = 0;
		uint32_t imageIndex;

		if (vkWaitForFences(device.Get(), 1, &fences[resourceIndex], VK_FALSE, 1000000000) != VK_SUCCESS)
		{
			std::cout << "Waiting for fence takes too long!" << std::endl;
		}
		vkResetFences(device.Get(), 1, &fences[resourceIndex]);

		vkAcquireNextImageKHR(device.Get(), swapchain.Get(), UINT64_MAX, imageAvailable[resourceIndex].Get(), VK_NULL_HANDLE, &imageIndex);
		CreateJustInTimeFramebuffer(framebuffers[resourceIndex], swapchainImageViews[imageIndex]);
		RecordJustInTimeCommandBuffers(resourceIndex);

		std::vector<VkCommandBuffer> commandBuffers = graphicsCommandBuffers.Get();

		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo =
		{
		  VK_STRUCTURE_TYPE_SUBMIT_INFO,                     // VkStructureType              sType
		  nullptr,                                           // const void                  *pNext
		  1,                                                 // uint32_t                     waitSemaphoreCount
		  imageAvailable[resourceIndex].GetPointer(),        // const VkSemaphore           *pWaitSemaphores
		  &waitDstStageMask,                                 // const VkPipelineStageFlags  *pWaitDstStageMask;
		  1,                                                 // uint32_t                     commandBufferCount
		  &commandBuffers[resourceIndex],                    // const VkCommandBuffer       *pCommandBuffers
		  1,                                                 // uint32_t                     signalSemaphoreCount
		  renderingFinished[resourceIndex].GetPointer()                  // const VkSemaphore           *pSignalSemaphores
		};

		if (vkQueueSubmit(queue, 1, &submitInfo, fences[resourceIndex]) != VK_SUCCESS)
		{
			std::cout << "Error while submitting queue" << std::endl;
		}

		VkPresentInfoKHR presentInfo =
		{
		  VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,                     // VkStructureType              sType
		  nullptr,                                                // const void                  *pNext
		  1,                                                      // uint32_t                     waitSemaphoreCount
		  renderingFinished[resourceIndex].GetPointer(),          // const VkSemaphore           *pWaitSemaphores
		  1,                                                      // uint32_t                     swapchainCount
		  swapchain.GetPointer(),                                 // const VkSwapchainKHR        *pSwapchains
		  &imageIndex,                                            // const uint32_t              *pImageIndices
		  nullptr                                                 // VkResult                    *pResults
		};

		vkQueuePresentKHR(queue, &presentInfo);

		resourceIndex = (resourceIndex + 1) % renderResourcesCount;
	}

	void PhongShading::RotateHorizontal(float angle)
	{
		//Rotation left means rotating around my up vector
		glm::vec3 forward = glm::normalize(eyeLocation - eyeDirection);
		glm::vec3 left = glm::cross(glm::normalize(up), forward);
		glm::vec3 actualUp = glm::cross(forward, left);
		glm::mat3 rotation = glm::rotate(glm::mat4(1.0f), angle, actualUp);
		eyeLocation = rotation * eyeLocation;
		//Now we need to recalculate up
		forward = glm::normalize(eyeLocation - eyeDirection);
		left = glm::cross(glm::normalize(up), forward);
		actualUp = glm::cross(forward, left);
		up = actualUp;
	}

	void PhongShading::RotateVertical(float angle)
	{
		//Rotation up, means rotation around my left or right vector
		// I need to define where left is, based on the current eye position:
		glm::vec3 forward = glm::normalize(eyeLocation - eyeDirection);
		glm::vec3 left = glm::cross(glm::normalize(up), forward);
		glm::mat3 rotation = glm::rotate(glm::mat4(1.0f), angle, left);
		eyeLocation = rotation * eyeLocation;
		//Now we need to recalculate up
		forward = glm::normalize(eyeLocation - eyeDirection);
		left = glm::cross(glm::normalize(up), forward);
		glm::vec3 actualUp = glm::cross(forward, left);
		up = actualUp;
	}

	PhongShading::PhongShading()
		: renderResourcesCount(3)
		, suitablePhysicalDeviceIndex(0xFFFFFFFF)
		, suitableQueueFamilyIndex(0xFFFFFFFF)
		, up(0.0f, 1.0f, 0.0f)
        , eyeLocation(0.0f, 0.0f, 3.0f)
	    , eyeDirection(0.0f, 0.0f, 0.0f)
	    , initialUp(0.0f, 1.0f, 0.0f)
	    , initialEyeLocation(0.0f, 0.0f, 3.0f)
	    , initialEyeDirection(0.0f, 0.0f, 0.0f)
		, leftPlane({0, 0, 0, 0})
		, rightPlane({ 0, 0, 0, 0 })
		, bottomPlane({ 0, 0, 0, 0 })
		, topPlane({ 0, 0, 0, 0 })
		, nearPlane({ 0, 0, 0, 0 })
		, farPlane({ 0, 0, 0, 0 })
	{
		WindowFactory windowFactory;
		window = windowFactory.Create(std::array<uint32_t, 2>{1920, 1080}, std::array<uint32_t, 2>{0, 0}, std::string{ "Vector Testing" });
		presentationExtensions = window->GetWindowExtensions();
		for (const auto& extension : presentationExtensions)
		{
			requestedExtensions[extension] = false;
		}

		try
		{
			CreateInstance();
			CreateSurface();
			CreateLogicalDevice();
			CreateSemaphores();
			CreateFences();
			CreateSwapchain();
			CreateRenderPass();
			CreateSwapchainImageViews();
			CreateDepthImage();
			CreateGraphicsCommandsBuffers();
			CreateDescriptorSetLayout();
			CreateDescriptorPool();
			AllocateDescriptorSet();
			CreateUniformBuffers();
			UpdateDescriptorSet();
			CreateVertexBuffer();
			CreateGraphicsPipeline();
			std::string modelPath(std::string(PHONG_PROJECT_CONTENT) + "Nyotengu.gltf");
			NyotenguModel.loadFromFile(modelPath, selectedPhysicalDevice, device.Get(), queue, graphicsCommandPool.Get(), 1.0f);
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	PhongShading::~PhongShading()
	{
	}
}

int pressDirection = 0;

void KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	  case GLFW_KEY_RIGHT: 
		  if (action == GLFW_PRESS)
		  {
			  std::cout << "Pressed Right" << std::endl;
			  pressDirection = 1;
		  }
		  break;
	  case GLFW_KEY_LEFT:
		  if (action == GLFW_PRESS)
		  {
			  std::cout << "Pressed Left" << std::endl;
			  pressDirection = 2;
		  }
		  break;
	  case GLFW_KEY_UP:
		  if (action == GLFW_PRESS)
		  {
			  std::cout << "Pressed Up" << std::endl;
			  pressDirection = 3;
		  }
		  break;
	  case GLFW_KEY_DOWN:
		  if (action == GLFW_PRESS)
		  {
			  std::cout << "Pressed Down" << std::endl;
			  pressDirection = 4;
		  }
		  break;
	  default: break;
	}
}

int main()
{
    std::cout << "Simple Phong Shading!" << std::endl;
    BadgerSandbox::PhongShading phongShading;
	// TODO: This is a horrible hack, create a service that propagates window events
	glfwSetKeyCallback((GLFWwindow*)phongShading.window->GetNativeWindow(), KeyCallBack);
	while (!phongShading.window->ShouldWindowClose())
	{
		switch (pressDirection)
		{
		case 1:
			pressDirection = 0;
			phongShading.RotateHorizontal(glm::radians(5.0f));
		break;
		case 2:
			pressDirection = 0;
			phongShading.RotateHorizontal(glm::radians(-5.0f));
			break;
		case 3:
			pressDirection = 0;
			phongShading.RotateVertical(glm::radians(5.0f));
			break;
		case 4:
			pressDirection = 0;
			phongShading.RotateVertical(glm::radians(-5.0f));
			break;
		default: break;

		}
		phongShading.Draw();
	}
    return 0;
}