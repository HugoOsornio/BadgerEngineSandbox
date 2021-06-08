#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "RenderBadger.h"

GLFWwindow* window;
VkInstance instance;
uint32_t suitablePhysicalDeviceIndex = 0xFFFFFFFF;
VkPhysicalDevice selectedPhysicalDevice;
VkDevice device;
uint32_t suitableQueueFamilyIndex = 0xFFFFFFFF;
VkQueue queue;
std::unordered_map<std::string, bool> requestedExtensions;
std::vector<VkExtensionProperties> availableExtensions;
VkSurfaceKHR surface;
VkSurfaceFormatKHR selectedSurfaceFormat;
VkExtent2D swapchainExtent;
VkSwapchainKHR swapchain;

std::vector<VkSemaphore> imageAvailable;
std::vector<VkSemaphore> renderingFinished;

VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

VkRenderPass renderPass;
std::vector<VkImageView> swapchainImageViews;
std::vector<VkFramebuffer> framebuffers;

VkShaderModule vertexShaderModule;
VkShaderModule fragmentShaderModule;

VkPipeline graphicsPipeline;

VkCommandPool graphicsCommandPool;
std::vector<VkCommandBuffer> graphicsCommandBuffers;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

static const size_t renderResourcesCount = 3;
std::vector<VkFence> fences;

std::vector<std::string> GetPresentationExtensions();

struct VertexData 
{
	float   x, y, z, w;
	float   u, v;
};

VkImage textureImage;
VkImageView textureImageView;
VkDeviceMemory textureMemory;
VkSampler textureSampler;

VkDescriptorSetLayout dsLayout;
VkDescriptorPool dPool;
VkDescriptorSet ds;
VkPipelineLayout pipelineLayout;


void CreateInstance()
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

	for (int i = 0; i < availableExtensionsCount; i++)
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

	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void CreateSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void CreateLogicalDevice()
{
	uint32_t numDevices = 0;
	if ((vkEnumeratePhysicalDevices(instance, &numDevices, nullptr) != VK_SUCCESS) ||
		(numDevices == 0))
	{
		throw std::runtime_error("Couldn't get a Physical Device");
	}

	std::vector<VkPhysicalDevice> physicalDevices(numDevices);
	if (vkEnumeratePhysicalDevices(instance, &numDevices, physicalDevices.data()) != VK_SUCCESS)
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
	  suitableQueueFamilyIndex,                    // uint32_t                     queueFamilyIndex
	  static_cast<uint32_t>(queuePriorities.size()),  // uint32_t                     queueCount
	  &queuePriorities[0]                            // const float                 *pQueuePriorities
	};

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo deviceCreateInfo = {
	  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,           // VkStructureType                    sType
	  nullptr,                                        // const void                        *pNext
	  0,                                              // VkDeviceCreateFlags                flags
	  1,                                              // uint32_t                           queueCreateInfoCount
	  &queueCreateInfo,                             // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
	  0,                                              // uint32_t                           enabledLayerCount
	  nullptr,                                        // const char * const                *ppEnabledLayerNames
	  deviceExtensions.size(),                        // uint32_t                           enabledExtensionCount
	  deviceExtensions.data(),                        // const char * const                *ppEnabledExtensionNames
	  nullptr                                         // const VkPhysicalDeviceFeatures    *pEnabledFeatures
	};

	if (vkCreateDevice(selectedPhysicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create a Logical Device");
	}

	vkGetDeviceQueue(device, suitableQueueFamilyIndex, 0, &queue);
}

void CreateSemaphores()
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

		if ((vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS) ||
			(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderingFinished[i]) != VK_SUCCESS))
		{
			throw std::runtime_error("Could not create a Swapchain semaphores");
		}
	}
}

void CreateFences()
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
		if (vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Couldn't create a fence");
		}
	}
}

void CreateSwapchain()
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
		if (sf.format == VK_FORMAT_R8G8B8A8_UNORM)
		{
			selectedSurfaceFormat = sf;
			break;
		}
	}
	if (selectedSurfaceFormat.format != VK_FORMAT_R8G8B8A8_UNORM)
	{
		std::cout << "Found an undefined format, forcing RGBA8888 UNORM";
		selectedSurfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
		selectedSurfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}

	// Special value of surface extent is width == height == 0xFFFFFFFF
	// If this is so we define the size by ourselves but it must fit within defined confines
	if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		int32_t width;
		int32_t height;
		glfwGetWindowSize(window, &width, &height);
		swapchainExtent.width = width;
		swapchainExtent.height = height;
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

	VkPresentModeKHR presentMode;
	for (VkPresentModeKHR& pm : presentModes)
	{
		if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = pm;
		}
	}
	if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
	{
		throw std::runtime_error("This sample requires a VK_PRESENT_MODE_MAILBOX_KHR capable swapchain");
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

	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create a Swapchain");
	}
	if (oldSwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
	}
}

void CreateRenderPass()
{
	std::array<VkAttachmentDescription, 1> attachmentDescriptions
	{
	 {
		0,                                   // VkAttachmentDescriptionFlags   flags
		selectedSurfaceFormat.format,        // VkFormat                       format
		VK_SAMPLE_COUNT_1_BIT,               // VkSampleCountFlagBits          samples
		VK_ATTACHMENT_LOAD_OP_CLEAR,         // VkAttachmentLoadOp             loadOp
		VK_ATTACHMENT_STORE_OP_STORE,        // VkAttachmentStoreOp            storeOp
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,     // VkAttachmentLoadOp             stencilLoadOp
		VK_ATTACHMENT_STORE_OP_DONT_CARE,    // VkAttachmentStoreOp            stencilStoreOp
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,     // VkImageLayout                  initialLayout;
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR      // VkImageLayout                  finalLayout
	 }
	};

	std::array<VkAttachmentReference, 1> colorAttachmentReferences
	{
	  {
		0,                                          // uint32_t                       attachment
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // VkImageLayout                  layout
	  }
	};

	std::array<VkSubpassDescription, 1> subpassesDescriptions
	{
	  {
		0,                                          // VkSubpassDescriptionFlags      flags
		VK_PIPELINE_BIND_POINT_GRAPHICS,            // VkPipelineBindPoint            pipelineBindPoint
		0,                                          // uint32_t                       inputAttachmentCount
		nullptr,                                    // const VkAttachmentReference   *pInputAttachments
		colorAttachmentReferences.size(),           // uint32_t                       colorAttachmentCount
		colorAttachmentReferences.data(),           // const VkAttachmentReference   *pColorAttachments
		nullptr,                                    // const VkAttachmentReference   *pResolveAttachments
		nullptr,                                    // const VkAttachmentReference   *pDepthStencilAttachment
		0,                                          // uint32_t                       preserveAttachmentCount
		nullptr                                     // const uint32_t*                pPreserveAttachments
	  }
	};

	std::vector<VkSubpassDependency> dependencies = 
	{
	  {
	    VK_SUBPASS_EXTERNAL,                            // uint32_t                       srcSubpass
	    0,                                              // uint32_t                       dstSubpass
	    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // VkPipelineStageFlags           srcStageMask
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags           dstStageMask
	    VK_ACCESS_MEMORY_READ_BIT,                      // VkAccessFlags                  srcAccessMask
	    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // VkAccessFlags                  dstAccessMask
	    VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags
	  },
	  {
	    0,                                              // uint32_t                       srcSubpass
	    VK_SUBPASS_EXTERNAL,                            // uint32_t                       dstSubpass
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags           srcStageMask
	    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // VkPipelineStageFlags           dstStageMask
	    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // VkAccessFlags                  srcAccessMask
	    VK_ACCESS_MEMORY_READ_BIT,                      // VkAccessFlags                  dstAccessMask
	    VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags
	  }
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

	if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Render Pass");
	}
}

void CreateSwapchainImageViews()
{
	uint32_t numberOfSwapchainImages = 0;
	vkGetSwapchainImagesKHR(device, swapchain, &numberOfSwapchainImages, nullptr);
	std::vector<VkImage> swapchainImages(numberOfSwapchainImages);
	vkGetSwapchainImagesKHR(device, swapchain, &numberOfSwapchainImages, swapchainImages.data());
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

		if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Couldn't create an image view for this swapchain image");
		}
	}
}

void CreateGraphicsPipeline()
{
	std::string vertexShaderPath(std::string(API_WITHOUT_SECRETS_PART6_CONTENT) + "vert.spv");
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

	if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &vertexShaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Vertex Shader Module");
	}

	std::string fragmentShaderPath(std::string(API_WITHOUT_SECRETS_PART6_CONTENT) + "frag.spv");
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


	if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &fragmentShaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Fragment Shader Module");
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos =
	{
	  {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
		nullptr,                                                    // const void                                    *pNext
		0,                                                          // VkPipelineShaderStageCreateFlags               flags
		VK_SHADER_STAGE_VERTEX_BIT,                                 // VkShaderStageFlagBits                          stage
		vertexShaderModule,                                         // VkShaderModule                                 module
		"main",                                                     // const char                                    *pName
		nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo  // Fragment shader
	  },
	  {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
		nullptr,                                                    // const void                                    *pNext
		0,                                                          // VkPipelineShaderStageCreateFlags               flags
		VK_SHADER_STAGE_FRAGMENT_BIT,                               // VkShaderStageFlagBits                          stage
		fragmentShaderModule,                                       // VkShaderModule                                 module
		"main",                                                     // const char                                    *pName
		nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo
	  }
	};

	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions = 
	{
	  {
		0,                                                          // uint32_t                                       binding
		sizeof(VertexData),                                         // uint32_t                                       stride
		VK_VERTEX_INPUT_RATE_VERTEX                                 // VkVertexInputRate                              inputRate
	  }
	};

	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = 
	{
	  {
		0,                                                          // uint32_t                                       location
		vertexInputBindingDescriptions[0].binding,                  // uint32_t                                       binding
		VK_FORMAT_R32G32B32A32_SFLOAT,                              // VkFormat                                       format
		offsetof(struct VertexData, x)                              // uint32_t                                       offset
	  },
	  {
		1,                                                          // uint32_t                                       location
		vertexInputBindingDescriptions[0].binding,                  // uint32_t                                       binding
		VK_FORMAT_R32G32_SFLOAT,                                    // VkFormat                                       format
		offsetof(struct VertexData, u)                              // uint32_t                                       offset
	  }
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
	  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,      // VkStructureType                                sType
	  nullptr,                                                        // const void                                    *pNext
	  0,                                                              // VkPipelineVertexInputStateCreateFlags          flags;
	  static_cast<uint32_t>(vertexInputBindingDescriptions.size()),   // uint32_t                                       vertexBindingDescriptionCount
	  &vertexInputBindingDescriptions[0],                             // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
	  static_cast<uint32_t>(vertexAttributeDescriptions.size()),      // uint32_t                                       vertexAttributeDescriptionCount
	  &vertexAttributeDescriptions[0]                                 // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
	};


	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
	{
	  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // VkStructureType                                sType
	  nullptr,                                                      // const void                                    *pNext
	  0,                                                            // VkPipelineInputAssemblyStateCreateFlags        flags
	  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                         // VkPrimitiveTopology                            topology
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
	  VK_CULL_MODE_BACK_BIT,                                        // VkCullModeFlags                                cullMode
	  VK_FRONT_FACE_COUNTER_CLOCKWISE,                              // VkFrontFace                                    frontFace
	  VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
	  0.0f,                                                         // float                                          depthBiasConstantFactor
	  0.0f,                                                         // float                                          depthBiasClamp
	  0.0f,                                                         // float                                          depthBiasSlopeFactor
	  1.0f                                                          // float                                          lineWidth
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

	VkPipelineLayoutCreateInfo layoutCreateInfo =
	{
	  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // VkStructureType                sType
	  nullptr,                                        // const void                    *pNext
	  0,                                              // VkPipelineLayoutCreateFlags    flags
	  1,                                              // uint32_t                       setLayoutCount
	  &dsLayout,                                      // const VkDescriptorSetLayout   *pSetLayouts
	  0,                                              // uint32_t                       pushConstantRangeCount
	  nullptr                                         // const VkPushConstantRange     *pPushConstantRanges
	};

	if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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
	  nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
	  &colorBlendStateCreateInfo,                                   // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
	  &dynamicStateCreateInfo,                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
	  pipelineLayout,                                               // VkPipelineLayout                               layout
	  renderPass,                                                   // VkRenderPass                                   renderPass
	  0,                                                            // uint32_t                                       subpass
	  VK_NULL_HANDLE,                                               // VkPipeline                                     basePipelineHandle
	  -1                                                            // int32_t                                        basePipelineIndex
	};

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Graphics Exception");
	}
}

void CreateGraphicsCommandsBuffers()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo =
	{
	  VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,           // VkStructureType                sType
	  nullptr,                                              // const void                    *pNext
	  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
	  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,                 // VkCommandPoolCreateFlags       flags
	  suitableQueueFamilyIndex                              // uint32_t                       queueFamilyIndex
	};

	if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create graphics command pools");
	}

	graphicsCommandBuffers.resize(renderResourcesCount);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo =
	{
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType                sType
	  nullptr,                                        // const void                    *pNext
	  graphicsCommandPool,                            // VkCommandPool                  commandPool
	  VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel           level
	  renderResourcesCount                            // uint32_t                       bufferCount
	};

	if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, graphicsCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create graphics command buffers");
	}

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

void AllocateBufferMemory(VkBuffer& buffer, const VkMemoryPropertyFlags& memoryProperty, VkDeviceMemory& memory)
{
	VkMemoryRequirements bufferMemoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(selectedPhysicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((bufferMemoryRequirements.memoryTypeBits & (1 << i)) &&
			((memoryProperties.memoryTypes[i].propertyFlags & memoryProperty) == memoryProperty))
		{

			VkMemoryAllocateInfo memoryAllocateInfo =
			{
			  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,       // VkStructureType                sType
			  nullptr,                                      // const void                    *pNext
			  bufferMemoryRequirements.size,              // VkDeviceSize                   allocationSize
			  i                                             // uint32_t                       memoryTypeIndex
			};

			if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
			{
				throw std::runtime_error("Could not Allocate memory for the buffer");
			}
		}
	}
}

void CreateBuffer(const VkDeviceSize& size, VkBufferUsageFlags usage, VkBuffer& bufferToCreate, VkDeviceMemory& memory, const VkMemoryPropertyFlags& memoryProperty)
{
	VkBufferCreateInfo bufferCreateInfo = {
	VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,             // VkStructureType                sType
	nullptr,                                          // const void                    *pNext
	0,                                                // VkBufferCreateFlags            flags
	size,                                      // VkDeviceSize                   size
	usage,                                            // VkBufferUsageFlags             usage
	VK_SHARING_MODE_EXCLUSIVE,                        // VkSharingMode                  sharingMode
	0,                                                // uint32_t                       queueFamilyIndexCount
	nullptr                                           // const uint32_t                *pQueueFamilyIndices
	};

	if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &bufferToCreate) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create buffer");
	}

	AllocateBufferMemory(bufferToCreate, memoryProperty, memory);

	if (vkBindBufferMemory(device, bufferToCreate, memory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not bind memory to buffer");
	}
}

void CreateVertexBuffer()
{
  VertexData vertexData[] =
  {
    {
	  -0.7f, -0.7f, 0.0f, 1.0f,
	  -0.1f, -0.1f
    },
    {
	  -0.7f, 0.7f, 0.0f, 1.0f,
	  -0.1f, 1.1f
    },
    {
	  0.7f, -0.7f, 0.0f, 1.0f,
	  1.1f, -0.1f
    },
    {
	  0.7f, 0.7f, 0.0f, 1.0f,
	  1.1f, 1.1f
    }
  };

	VkDeviceSize vertexBufferSize = sizeof(vertexData);
	CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertexBuffer, vertexBufferMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkBuffer stagingBuffer;
	VkDeviceSize stagingBufferSize = 4000;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer, stagingBufferMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	void* stagingBufferMemoryPointer;
	if (vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &stagingBufferMemoryPointer) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not map staging buffer memory");
	}

	memcpy(stagingBufferMemoryPointer, vertexData, vertexBufferSize);

	VkMappedMemoryRange flushRange = 
	{
	  VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,            // VkStructureType                        sType
	  nullptr,                                          // const void                            *pNext
	  stagingBufferMemory,                              // VkDeviceMemory                         memory
	  0,                                                // VkDeviceSize                           offset
	  vertexBufferSize                                  // VkDeviceSize                           size
	};
	vkFlushMappedMemoryRanges(device, 1, &flushRange);

	vkUnmapMemory(device, stagingBufferMemory);

	// Prepare command buffer to copy data from staging buffer to a vertex buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = 
	{
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,      // VkStructureType                        sType
	  nullptr,                                          // const void                            *pNext
	  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,      // VkCommandBufferUsageFlags              flags
	  nullptr                                           // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
	};

	VkCommandBuffer commandBuffer = graphicsCommandBuffers[0];

	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

	VkBufferCopy bufferCopyInfo = 
	{
	  0,                                                // VkDeviceSize                           srcOffset
	  0,                                                // VkDeviceSize                           dstOffset
	  vertexBufferSize                                  // VkDeviceSize                           size
	};
	vkCmdCopyBuffer(commandBuffer, stagingBuffer, vertexBuffer, 1, &bufferCopyInfo);

	VkBufferMemoryBarrier bufferMemoryBarrier = 
	{
	  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,          // VkStructureType                        sType;
	  nullptr,                                          // const void                            *pNext
	  VK_ACCESS_MEMORY_WRITE_BIT,                       // VkAccessFlags                          srcAccessMask
	  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,              // VkAccessFlags                          dstAccessMask
	  VK_QUEUE_FAMILY_IGNORED,                          // uint32_t                               srcQueueFamilyIndex
	  VK_QUEUE_FAMILY_IGNORED,                          // uint32_t                               dstQueueFamilyIndex
	  vertexBuffer,                                     // VkBuffer                               buffer
	  0,                                                // VkDeviceSize                           offset
	  VK_WHOLE_SIZE                                     // VkDeviceSize                           size
	};
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);

	vkEndCommandBuffer(commandBuffer);

	// Submit command buffer and copy data from staging buffer to a vertex buffer
	VkSubmitInfo submitInfo = 
	{
	  VK_STRUCTURE_TYPE_SUBMIT_INFO,                    // VkStructureType                        sType
	  nullptr,                                          // const void                            *pNext
	  0,                                                // uint32_t                               waitSemaphoreCount
	  nullptr,                                          // const VkSemaphore                     *pWaitSemaphores
	  nullptr,                                          // const VkPipelineStageFlags            *pWaitDstStageMask;
	  1,                                                // uint32_t                               commandBufferCount
	  & commandBuffer,                                  // const VkCommandBuffer                 *pCommandBuffers
	  0,                                                // uint32_t                               signalSemaphoreCount
	  nullptr                                           // const VkSemaphore                     *pSignalSemaphores
	};

	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) 
	{
		throw std::runtime_error("Copy from staging to vertex buffer command failed");
	}

	vkDeviceWaitIdle(device);

}

void CreateJustInTimeFramebuffer(VkFramebuffer& framebuffer, const VkImageView& imageView)
{
	if (framebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}

	VkFramebufferCreateInfo framebufferCreateInfo = 
	{
	  VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,      // VkStructureType                sType
	  nullptr,                                        // const void                    *pNext
	  0,                                              // VkFramebufferCreateFlags       flags
	  renderPass,                                     // VkRenderPass                   renderPass
	  1,                                              // uint32_t                       attachmentCount
	  &imageView,                                     // const VkImageView             *pAttachments
	  swapchainExtent.width,                          // uint32_t                       width
	  swapchainExtent.height,                         // uint32_t                       height
	  1                                               // uint32_t                       layers
	};
	
	if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS)
	{
		std::cout << "Could not create a framebuffer!" << std::endl;
	}
}

void RecordJustInTimeCommandBuffers(const size_t& resourceIndex)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo = 
	{
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        // VkCommandBufferUsageFlags              flags
	  nullptr                                             // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
	};

	vkBeginCommandBuffer(graphicsCommandBuffers[resourceIndex], &commandBufferBeginInfo);

	VkImageSubresourceRange image_subresource_range = 
	{
	  VK_IMAGE_ASPECT_COLOR_BIT,                          // VkImageAspectFlags                     aspectMask
	  0,                                                  // uint32_t                               baseMipLevel
	  1,                                                  // uint32_t                               levelCount
	  0,                                                  // uint32_t                               baseArrayLayer
	  1                                                   // uint32_t                               layerCount
	};

	VkClearValue clearValue = 
	{
	  { 1.0f, 0.8f, 0.4f, 0.0f },                         // VkClearColorValue                      color
	};

	VkRenderPassBeginInfo renderPassBeginInfo = 
	{
	  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,           // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  renderPass,                                         // VkRenderPass                           renderPass
	  framebuffers[resourceIndex],                        // VkFramebuffer                          framebuffer
	  {                                                   // VkRect2D                               renderArea
		{                                                 // VkOffset2D                             offset
		  0,                                              // int32_t                                x
		  0                                               // int32_t                                y
		},
		swapchainExtent,                                  // VkExtent2D                             extent;
	  },
	  1,                                                  // uint32_t                               clearValueCount
	  &clearValue                                         // const VkClearValue                    *pClearValues
	};

	vkCmdBeginRenderPass(graphicsCommandBuffers[resourceIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(graphicsCommandBuffers[resourceIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

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

	vkCmdSetViewport(graphicsCommandBuffers[resourceIndex], 0, 1, &viewport);
	vkCmdSetScissor(graphicsCommandBuffers[resourceIndex], 0, 1, &scissor);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(graphicsCommandBuffers[resourceIndex], 0, 1, &vertexBuffer, &offset);
	vkCmdBindDescriptorSets(graphicsCommandBuffers[resourceIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &ds, 0, nullptr);
	vkCmdDraw(graphicsCommandBuffers[resourceIndex], 4, 1, 0, 0);

	vkCmdEndRenderPass(graphicsCommandBuffers[resourceIndex]);

	if (vkEndCommandBuffer(graphicsCommandBuffers[resourceIndex]) != VK_SUCCESS) 
	{
		std::cout << "Could not record command buffer!" << std::endl;
	}
}

void CreateImage(uint32_t width, uint32_t height, VkImage& image)
{
	VkImageCreateInfo imageCreateInfo = 
	{
	  VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // VkStructureType        sType;
	  nullptr,                              // const void            *pNext
	  0,                                    // VkImageCreateFlags     flags
	  VK_IMAGE_TYPE_2D,                     // VkImageType            imageType
	  VK_FORMAT_R8G8B8A8_UNORM,             // VkFormat               format
	  {                                     // VkExtent3D             extent
	    width,                                // uint32_t               width
	    height,                               // uint32_t               height
	    1                                     // uint32_t               depth
	  },
	  1,                                    // uint32_t               mipLevels
	  1,                                    // uint32_t               arrayLayers
	  VK_SAMPLE_COUNT_1_BIT,                // VkSampleCountFlagBits  samples
	  VK_IMAGE_TILING_OPTIMAL,              // VkImageTiling          tiling
	  VK_IMAGE_USAGE_TRANSFER_DST_BIT |     // VkImageUsageFlags      usage
	  VK_IMAGE_USAGE_SAMPLED_BIT,
	  VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode          sharingMode
	  0,                                    // uint32_t               queueFamilyIndexCount
	  nullptr,                              // const uint32_t        *pQueueFamilyIndices
	  VK_IMAGE_LAYOUT_UNDEFINED             // VkImageLayout          initialLayout
	};

	if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create Image");
	}
}

void AllocateImageMemory(const VkImage& image, VkDeviceMemory& memory, const VkMemoryPropertyFlags& memoryProperty)
{
	VkMemoryRequirements imageMemoryRequirements;
	vkGetImageMemoryRequirements(device, image, &imageMemoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(selectedPhysicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) 
	{
		if ((imageMemoryRequirements.memoryTypeBits & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & memoryProperty)) 
		{

			VkMemoryAllocateInfo memory_allocate_info = 
			{
			  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // VkStructureType  sType
			  nullptr,                                // const void      *pNext
			  imageMemoryRequirements.size,           // VkDeviceSize     allocationSize
			  i                                       // uint32_t         memoryTypeIndex
			};

			if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS) 
			{
				throw std::runtime_error("Couldn't Allocate Memory for the image");
			}
			else
			{
				break;
			}
		}
	}
}

void CreateImageView(const VkImage& image, VkImageView& imageView)
{
	VkImageViewCreateInfo imageViewCreateInfo = 
	{
	  VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // VkStructureType          sType
	  nullptr,                                  // const void              *pNext
	  0,                                        // VkImageViewCreateFlags   flags
	  image,                                    // VkImage                  image
	  VK_IMAGE_VIEW_TYPE_2D,                    // VkImageViewType          viewType
	  VK_FORMAT_R8G8B8A8_UNORM,                 // VkFormat                 format
	  {                                         // VkComponentMapping       components
	    VK_COMPONENT_SWIZZLE_IDENTITY,            // VkComponentSwizzle       r
	    VK_COMPONENT_SWIZZLE_IDENTITY,            // VkComponentSwizzle       g
	    VK_COMPONENT_SWIZZLE_IDENTITY,            // VkComponentSwizzle       b
	    VK_COMPONENT_SWIZZLE_IDENTITY             // VkComponentSwizzle       a
	  },
	  {                                         // VkImageSubresourceRange  subresourceRange
	    VK_IMAGE_ASPECT_COLOR_BIT,                // VkImageAspectFlags       aspectMask
	    0,                                        // uint32_t                 baseMipLevel
	    1,                                        // uint32_t                 levelCount
	    0,                                        // uint32_t                 baseArrayLayer
	    1                                         // uint32_t                 layerCount
	  }
	};

	if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create image view");
	}
}

void CreateSampler(VkSampler& sampler)
{
	VkSamplerCreateInfo samplerCreateInfo = 
	{
	  VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,                // VkStructureType            sType
	  nullptr,                                              // const void*                pNext
	  0,                                                    // VkSamplerCreateFlags       flags
	  VK_FILTER_LINEAR,                                     // VkFilter                   magFilter
	  VK_FILTER_LINEAR,                                     // VkFilter                   minFilter
	  VK_SAMPLER_MIPMAP_MODE_NEAREST,                       // VkSamplerMipmapMode        mipmapMode
	  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,                // VkSamplerAddressMode       addressModeU
	  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,                // VkSamplerAddressMode       addressModeV
	  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,                // VkSamplerAddressMode       addressModeW
	  0.0f,                                                 // float                      mipLodBias
	  VK_FALSE,                                             // VkBool32                   anisotropyEnable
	  1.0f,                                                 // float                      maxAnisotropy
	  VK_FALSE,                                             // VkBool32                   compareEnable
	  VK_COMPARE_OP_ALWAYS,                                 // VkCompareOp                compareOp
	  0.0f,                                                 // float                      minLod
	  0.0f,                                                 // float                      maxLod
	  VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,              // VkBorderColor              borderColor
	  VK_FALSE                                              // VkBool32                   unnormalizedCoordinates
	};

	if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create sampler");
	}
}

void CreateTexture()
{
	CreateImage(720, 720, textureImage);
	AllocateImageMemory(textureImage, textureMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if(vkBindImageMemory(device, textureImage, textureMemory, 0) != VK_SUCCESS)
	{
	  throw std::runtime_error("Couldn't bind memory to texture");
	}

	CreateImageView(textureImage, textureImageView);
	CreateSampler(textureSampler);
	
	VkBuffer stagingBuffer;
	VkDeviceSize stagingBufferSize = 720*720*4;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer, stagingBufferMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	void* stagingBufferMemoryPointer;

	if (vkMapMemory(device, stagingBufferMemory, 0, stagingBufferSize, 0, &stagingBufferMemoryPointer) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't map memory to the Image staging buffer");
	}

	memcpy(stagingBufferMemoryPointer, ImageData, stagingBufferSize);

	VkMappedMemoryRange flushRange = 
	{
	  VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,              // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  stagingBufferMemory,                                // VkDeviceMemory                         memory
	  0,                                                  // VkDeviceSize                           offset
	  stagingBufferSize                                   // VkDeviceSize                           size
	};
	vkFlushMappedMemoryRanges(device, 1, &flushRange);
	vkUnmapMemory(device, stagingBufferMemory);

	// Prepare command buffer to copy data from staging buffer to a vertex buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = 
	{
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        // VkCommandBufferUsageFlags              flags
	  nullptr                                             // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
	};

	VkCommandBuffer commandBuffer = graphicsCommandBuffers[0];

	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

	VkImageSubresourceRange imageSubResourceRange = 
	{
	  VK_IMAGE_ASPECT_COLOR_BIT,                          // VkImageAspectFlags                     aspectMask
	  0,                                                  // uint32_t                               baseMipLevel
	  1,                                                  // uint32_t                               levelCount
	  0,                                                  // uint32_t                               baseArrayLayer
	  1                                                   // uint32_t                               layerCount
	};

	VkImageMemoryBarrier imageMemoryBarrierFromUndefinedToTransferDst = 
	{
	  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,             // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  0,                                                  // VkAccessFlags                          srcAccessMask
	  VK_ACCESS_TRANSFER_WRITE_BIT,                       // VkAccessFlags                          dstAccessMask
	  VK_IMAGE_LAYOUT_UNDEFINED,                          // VkImageLayout                          oldLayout
	  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,               // VkImageLayout                          newLayout
	  VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               srcQueueFamilyIndex
	  VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               dstQueueFamilyIndex
	  textureImage,                                // VkImage                                image
	  imageSubResourceRange                             // VkImageSubresourceRange                subresourceRange
	};
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierFromUndefinedToTransferDst);

	VkBufferImageCopy bufferImageCopyInfo = 
	{
	  0,                                                  // VkDeviceSize                           bufferOffset
	  0,                                                  // uint32_t                               bufferRowLength
	  0,                                                  // uint32_t                               bufferImageHeight
	  {                                                   // VkImageSubresourceLayers               imageSubresource
		VK_IMAGE_ASPECT_COLOR_BIT,                          // VkImageAspectFlags                     aspectMask
		0,                                                  // uint32_t                               mipLevel
		0,                                                  // uint32_t                               baseArrayLayer
		1                                                   // uint32_t                               layerCount
	  },
	  {                                                   // VkOffset3D                             imageOffset
		0,                                                  // int32_t                                x
		0,                                                  // int32_t                                y
		0                                                   // int32_t                                z
	  },
	  {                                                   // VkExtent3D                             imageExtent
		720,                                                // uint32_t                               width
		720,                                                // uint32_t                               height
		1                                                   // uint32_t                               depth
	  }
	};
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopyInfo);

	VkImageMemoryBarrier imageMemoryBarrierFromTransferToShaderRead = 
	{
	  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,             // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  VK_ACCESS_TRANSFER_WRITE_BIT,                       // VkAccessFlags                          srcAccessMask
	  VK_ACCESS_SHADER_READ_BIT,                          // VkAccessFlags                          dstAccessMask
	  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,               // VkImageLayout                          oldLayout
	  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,           // VkImageLayout                          newLayout
	  VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               srcQueueFamilyIndex
	  VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               dstQueueFamilyIndex
	  textureImage,                                       // VkImage                                image
	  imageSubResourceRange                               // VkImageSubresourceRange                subresourceRange
	};
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierFromTransferToShaderRead);

	vkEndCommandBuffer(commandBuffer);

	// Submit command buffer and copy data from staging buffer to a vertex buffer
	VkSubmitInfo submitInfo = 
	{
	  VK_STRUCTURE_TYPE_SUBMIT_INFO,                      // VkStructureType                        sType
	  nullptr,                                            // const void                            *pNext
	  0,                                                  // uint32_t                               waitSemaphoreCount
	  nullptr,                                            // const VkSemaphore                     *pWaitSemaphores
	  nullptr,                                            // const VkPipelineStageFlags            *pWaitDstStageMask;
	  1,                                                  // uint32_t                               commandBufferCount
	  &commandBuffer,                                     // const VkCommandBuffer                 *pCommandBuffers
	  0,                                                  // uint32_t                               signalSemaphoreCount
	  nullptr                                             // const VkSemaphore                     *pSignalSemaphores
	};

	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) 
	{
		throw std::runtime_error("Command Buffer to copy from staging buffer to Image buffer memory failed");
	}

	vkDeviceWaitIdle(device);
}

void CreateDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding layoutBinding = 
  {
    0,                                                    // uint32_t                             binding
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,            // VkDescriptorType                     descriptorType
	1,                                                    // uint32_t                             descriptorCount
	VK_SHADER_STAGE_FRAGMENT_BIT,                         // VkShaderStageFlags                   stageFlags
	nullptr                                               // const VkSampler                     *pImmutableSamplers
  };

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = 
  {
	VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,  // VkStructureType                      sType
	nullptr,                                              // const void                          *pNext
	0,                                                    // VkDescriptorSetLayoutCreateFlags     flags
	1,                                                    // uint32_t                             bindingCount
	& layoutBinding                                       // const VkDescriptorSetLayoutBinding  *pBindings
  };

  if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &dsLayout) != VK_SUCCESS) 
  {
	  throw std::runtime_error("Could not create descriptor set layout!");
  }
}

void CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize = 
	{
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,      // VkDescriptorType               type
	  1                                               // uint32_t                       descriptorCount
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = 
	{
	  VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,  // VkStructureType                sType
	  nullptr,                                        // const void                    *pNext
	  0,                                              // VkDescriptorPoolCreateFlags    flags
	  1,                                              // uint32_t                       maxSets
	  1,                                              // uint32_t                       poolSizeCount
	  &poolSize                                      // const VkDescriptorPoolSize    *pPoolSizes
	};

	if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &dPool) != VK_SUCCESS) 
	{
		throw std::runtime_error("Could not create descriptor pool!");
	}
}

void AllocateDescriptorSet()
{
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = 
  {
	VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // VkStructureType                sType
	nullptr,                                        // const void                    *pNext
	dPool,                                          // VkDescriptorPool               descriptorPool
	1,                                              // uint32_t                       descriptorSetCount
	&dsLayout                                       // const VkDescriptorSetLayout   *pSetLayouts
	};

	if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &ds) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not allocate descriptor set!");
	}
}

void UpdateDescriptorSet()
{
	VkDescriptorImageInfo imageInfo = 
	{
	  textureSampler,                             // VkSampler                      sampler
	  textureImageView,                           // VkImageView                    imageView
	  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL    // VkImageLayout                  imageLayout
	};

	VkWriteDescriptorSet descriptorWrites = 
	{
	  VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,     // VkStructureType                sType
	  nullptr,                                    // const void                    *pNext
	  ds,                                         // VkDescriptorSet                dstSet
	  0,                                          // uint32_t                       dstBinding
	  0,                                          // uint32_t                       dstArrayElement
	  1,                                          // uint32_t                       descriptorCount
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // VkDescriptorType               descriptorType
	  &imageInfo,                                 // const VkDescriptorImageInfo   *pImageInfo
	  nullptr,                                    // const VkDescriptorBufferInfo  *pBufferInfo
	  nullptr                                     // const VkBufferView            *pTexelBufferView
	};

	vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr);
}

void Draw()
{
	static size_t resourceIndex = 0;
	uint32_t imageIndex;

	if (vkWaitForFences(device, 1, &fences[resourceIndex], VK_FALSE, 1000000000) != VK_SUCCESS) 
	{
		std::cout << "Waiting for fence takes too long!" << std::endl;
	}
	vkResetFences(device, 1, &fences[resourceIndex]);

	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable[resourceIndex], VK_NULL_HANDLE, &imageIndex);
	CreateJustInTimeFramebuffer(framebuffers[resourceIndex], swapchainImageViews[imageIndex]);
	RecordJustInTimeCommandBuffers(resourceIndex);

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = 
	{
	  VK_STRUCTURE_TYPE_SUBMIT_INFO,                     // VkStructureType              sType
	  nullptr,                                           // const void                  *pNext
	  1,                                                 // uint32_t                     waitSemaphoreCount
	  &imageAvailable[resourceIndex],                    // const VkSemaphore           *pWaitSemaphores
	  &waitDstStageMask,                                 // const VkPipelineStageFlags  *pWaitDstStageMask;
	  1,                                                 // uint32_t                     commandBufferCount
	  &graphicsCommandBuffers[resourceIndex],            // const VkCommandBuffer       *pCommandBuffers
	  1,                                                 // uint32_t                     signalSemaphoreCount
	  &renderingFinished[resourceIndex]                  // const VkSemaphore           *pSignalSemaphores
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
	  &renderingFinished[resourceIndex],                      // const VkSemaphore           *pWaitSemaphores
	  1,                                                      // uint32_t                     swapchainCount
	  &swapchain,                                             // const VkSwapchainKHR        *pSwapchains
	  &imageIndex,                                            // const uint32_t              *pImageIndices
	  nullptr                                                 // VkResult                    *pResults
	};

	vkQueuePresentKHR(queue, &presentInfo);

	resourceIndex = (resourceIndex + 1) % renderResourcesCount;
}

void initVulkan()
{
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
		CreateGraphicsCommandsBuffers();
		CreateVertexBuffer();
		CreateTexture();
		CreateDescriptorSetLayout();
		CreateDescriptorPool();
		AllocateDescriptorSet();
		UpdateDescriptorSet();
		CreateGraphicsPipeline();
	}
	catch (...)
	{

	}
}

void initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(1920, 1080, "ApiWithoutSecrets_Part6", nullptr, nullptr);
	std::vector<std::string> presentationExtensions = GetPresentationExtensions();
	for (const auto& extension : presentationExtensions)
	{
		requestedExtensions[extension] = false;
	}
	//glfwSetWindowUserPointer(window, this);
	//glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

std::vector<std::string> GetPresentationExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<std::string> localExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	return localExtensions;
}

int main()
{
	std::cout << "Hello World!" << std::endl;
	initWindow();
	initVulkan();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		Draw();
	}

	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);

		if ((commandBuffers.size() > 0) && (commandBuffers[0] != VK_NULL_HANDLE))
		{
			vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
			commandBuffers.clear();
		}

		if (commandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, commandPool, nullptr);
			commandPool = VK_NULL_HANDLE;
		}

		for (auto& semaphore : imageAvailable)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device, semaphore, nullptr);
			}
		}

		for (auto& semaphore : renderingFinished)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device, semaphore, nullptr);
			}
		}

		if (swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, swapchain, nullptr);
		}

		vkDestroyDevice(device, nullptr);
	}

}