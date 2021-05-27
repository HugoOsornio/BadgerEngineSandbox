#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

VkSemaphore imageAvailable;
VkSemaphore renderingFinished;

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

std::vector<std::string> GetPresentationExtensions();

#define STAND_ALONE_CONTENT_PATH "F:\RenderBadger\VulkanBadger\BadgerEngineSandbox\SelfContainedSamples\ApiWithoutSecrets_Part3Content"

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
    & queuePriorities[0]                            // const float                 *pQueuePriorities
  };

  std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  VkDeviceCreateInfo deviceCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,           // VkStructureType                    sType
    nullptr,                                        // const void                        *pNext
    0,                                              // VkDeviceCreateFlags                flags
    1,                                              // uint32_t                           queueCreateInfoCount
    & queueCreateInfo,                             // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
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

void CreateSwapchain()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = 
    {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,      // VkStructureType          sType
      nullptr,                                      // const void*              pNext
      0                                             // VkSemaphoreCreateFlags   flags
    };

    if ((vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailable) != VK_SUCCESS) ||
        (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderingFinished) != VK_SUCCESS)) 
    {
        throw std::runtime_error("Could not create a Swapchain semaphores");
    }

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

    VkRenderPassCreateInfo renderPassCreateInfo = 
    {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,    // VkStructureType                sType
      nullptr,                                      // const void                    *pNext                                           
      0,                                            // VkRenderPassCreateFlags        flags
      attachmentDescriptions.size(),                // uint32_t                       attachmentCount
      attachmentDescriptions.data(),                // const VkAttachmentDescription *pAttachments
      subpassesDescriptions.size(),                 // uint32_t                       subpassCount
      subpassesDescriptions.data(),                 // const VkSubpassDescription    *pSubpasses
      0,                                            // uint32_t                       dependencyCount
      nullptr                                       // const VkSubpassDependency     *pDependencies
    }; 
    
    if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create Render Pass");
    }
}

void CreateFramebuffers()
{
    uint32_t numberOfSwapchainImages = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &numberOfSwapchainImages, nullptr);
    std::vector<VkImage> swapchainImages(numberOfSwapchainImages);
    vkGetSwapchainImagesKHR(device, swapchain, &numberOfSwapchainImages, swapchainImages.data());
    swapchainImageViews.resize(numberOfSwapchainImages);
    framebuffers.resize(numberOfSwapchainImages);
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

        VkFramebufferCreateInfo framebufferCreateInfo = 
        {
          VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // VkStructureType                sType
          nullptr,                                    // const void                    *pNext
          0,                                          // VkFramebufferCreateFlags       flags
          renderPass,                                 // VkRenderPass                   renderPass
          1,                                          // uint32_t                       attachmentCount
          &swapchainImageViews[i],                    // const VkImageView             *pAttachments
          swapchainExtent.width,                      // uint32_t                       width
          swapchainExtent.height,                     // uint32_t                       height
          1                                           // uint32_t                       layers
        };

        if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Couldn't create a Framebuffer for this device");
        }
    }
}

void CreateGraphicsPipeline()
{
    std::string vertexShaderPath(std::string(API_WITHOUT_SECRETS_PART3_CONTENT) + "vert.spv");
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

    std::string fragmentShaderPath(std::string(API_WITHOUT_SECRETS_PART3_CONTENT) + "frag.spv");
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

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = 
    {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,    // VkStructureType                                sType
      nullptr,                                                      // const void                                    *pNext
      0,                                                            // VkPipelineVertexInputStateCreateFlags          flags;
      0,                                                            // uint32_t                                       vertexBindingDescriptionCount
      nullptr,                                                      // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
      0,                                                            // uint32_t                                       vertexAttributeDescriptionCount
      nullptr                                                       // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = 
    {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // VkStructureType                                sType
      nullptr,                                                      // const void                                    *pNext
      0,                                                            // VkPipelineInputAssemblyStateCreateFlags        flags
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                          // VkPrimitiveTopology                            topology
      VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
    };

    VkViewport viewport = 
    {
      0.0f,                                                         // float                                          x
      0.0f,                                                         // float                                          y
      float(swapchainExtent.width),                                 // float                                          width
      float(swapchainExtent.height),                                // float                                          height
      0.0f,                                                         // float                                          minDepth
      1.0f                                                          // float                                          maxDepth
    };

    VkRect2D scissor = 
    {
      VkOffset2D{0, 0},
      swapchainExtent
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = 
    {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,        // VkStructureType                                sType
      nullptr,                                                      // const void                                    *pNext
      0,                                                            // VkPipelineViewportStateCreateFlags             flags
      1,                                                            // uint32_t                                       viewportCount
      &viewport,                                                    // const VkViewport                              *pViewports
      1,                                                            // uint32_t                                       scissorCount
      &scissor                                                      // const VkRect2D                                *pScissors
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
      & colorBlendAttachmentState,                                  // const VkPipelineColorBlendAttachmentState     *pAttachments
      { 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
    };

    VkPipelineLayoutCreateInfo layoutCreateInfo = 
    {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // VkStructureType                sType
      nullptr,                                        // const void                    *pNext
      0,                                              // VkPipelineLayoutCreateFlags    flags
      0,                                              // uint32_t                       setLayoutCount
      nullptr,                                        // const VkDescriptorSetLayout   *pSetLayouts
      0,                                              // uint32_t                       pushConstantRangeCount
      nullptr                                         // const VkPushConstantRange     *pPushConstantRanges
    };

    VkPipelineLayout pipelineLayout;
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
      nullptr,                                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
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
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,     // VkStructureType                sType
      nullptr,                                        // const void                    *pNext
      0,                                              // VkCommandPoolCreateFlags       flags
      suitableQueueFamilyIndex                              // uint32_t                       queueFamilyIndex
    };

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS) 
    {
        throw std::runtime_error("Could not create graphics command pools");
    }

    uint32_t imageCount = 0;
    if ((vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS) ||
        (imageCount == 0))
    {
        throw std::runtime_error("Could not get the image count from swapchain");
    }

    graphicsCommandBuffers.resize(imageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = 
    {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType                sType
      nullptr,                                        // const void                    *pNext
      graphicsCommandPool,                            // VkCommandPool                  commandPool
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel           level
      imageCount                                      // uint32_t                       bufferCount
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

    for (size_t i = 0; i < graphicsCommandBuffers.size(); ++i)
    {
      vkBeginCommandBuffer(graphicsCommandBuffers[i], &graphicsCommandBufferBeginInfo);
      VkRenderPassBeginInfo renderPassBeginInfo = 
      {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,     // VkStructureType                sType
        nullptr,                                      // const void                    *pNext
        renderPass,                                   // VkRenderPass                   renderPass
        framebuffers[i],                              // VkFramebuffer                  framebuffer
        {                                             // VkRect2D                       renderArea
          {                                           // VkOffset2D                     offset
            0,                                        // int32_t                        x
            0                                         // int32_t                        y
          },
          {                                           // VkExtent2D                     extent
            swapchainExtent
          }
        },
        1,                                            // uint32_t                       clearValueCount
        &clearValue                                   // const VkClearValue            *pClearValues
      };

      vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
      vkCmdDraw(graphicsCommandBuffers[i], 3, 1, 0, 0);
      vkCmdEndRenderPass(graphicsCommandBuffers[i]);
      if (vkEndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS) 
      {
          throw std::runtime_error("Couldn't record Graphics Command Buffers");
      }
    }
}

void Draw()
{
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
    switch (result) 
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    default:
        std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
    }
    
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submitInfo = 
    {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,                // VkStructureType              sType
      nullptr,                                      // const void                  *pNext
      1,                                            // uint32_t                     waitSemaphoreCount
      &imageAvailable,                              // const VkSemaphore           *pWaitSemaphores
      &waitDstStageMask,                            // const VkPipelineStageFlags  *pWaitDstStageMask;
      1,                                            // uint32_t                     commandBufferCount
      &graphicsCommandBuffers[imageIndex],          // const VkCommandBuffer       *pCommandBuffers
      1,                                            // uint32_t                     signalSemaphoreCount
      &renderingFinished                            // const VkSemaphore           *pSignalSemaphores
    };

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) 
    {
        std::cout << "Error while submitting queue" << std::endl;
    }

    VkPresentInfoKHR presentInfo = 
    {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,           // VkStructureType              sType
      nullptr,                                      // const void                  *pNext
      1,                                            // uint32_t                     waitSemaphoreCount
      &renderingFinished,                           // const VkSemaphore           *pWaitSemaphores
      1,                                            // uint32_t                     swapchainCount
      &swapchain,                                   // const VkSwapchainKHR        *pSwapchains
      &imageIndex,                                  // const uint32_t              *pImageIndices
      nullptr                                       // VkResult                    *pResults
    };
    result = vkQueuePresentKHR(queue, &presentInfo);

    switch (result) 
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
    default:
        std::cout << "Problem occurred during image presentation!" << std::endl;
    }
}

void initVulkan() 
{
    try
    {
      CreateInstance();
      CreateSurface();
      CreateLogicalDevice();
      CreateSwapchain();
      CreateRenderPass();
      CreateFramebuffers();
      CreateGraphicsPipeline();
      CreateGraphicsCommandsBuffers();
    }
    catch (...)
    {
    
    }
}

void initWindow() 
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "ApiWithoutSecrets_Part3", nullptr, nullptr);
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

     if (imageAvailable != VK_NULL_HANDLE) 
     {
       vkDestroySemaphore(device, imageAvailable, nullptr);
     }
     
     if (renderingFinished != VK_NULL_HANDLE) 
     {
       vkDestroySemaphore(device, renderingFinished, nullptr);
     }
     
     if (swapchain != VK_NULL_HANDLE) 
     {
       vkDestroySwapchainKHR(device, swapchain, nullptr);
     }

     vkDestroyDevice(device, nullptr);
  }

}