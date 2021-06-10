#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include <string>

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


std::vector<std::string> GetPresentationExtensions();

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
      if (sf.format == VK_FORMAT_B8G8R8A8_UNORM) 
      {
        selectedSurfaceFormat = sf;
        break;
      }
    }
    if (selectedSurfaceFormat.format != VK_FORMAT_B8G8R8A8_UNORM)
    {
        std::cout << "Found an undefined format, forcing VK_FORMAT_B8G8R8A8_UNORM";
        selectedSurfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
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

    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) 
    {
        throw std::runtime_error("Couldn't create a Swapchain");
    }
    if (oldSwapChain != VK_NULL_HANDLE) 
    {
        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    }
}

void CreateCommandBuffers()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = 
    {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,     // VkStructureType              sType
      nullptr,                                        // const void*                  pNext
      0,                                              // VkCommandPoolCreateFlags     flags
      suitableQueueFamilyIndex                        // uint32_t                     queueFamilyIndex
    };

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) 
    {
        throw std::runtime_error("Could not create command Pool");
    }

    uint32_t imageCount = 0;
    if ((vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS) ||
        (imageCount == 0))
    {
        throw std::runtime_error("Could not get the image count from swapchain");
    }

    commandBuffers.resize(imageCount);

    VkCommandBufferAllocateInfo cmdBufferAllocateInfo = 
    {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType              sType
      nullptr,                                        // const void*                  pNext
      commandPool,                                    // VkCommandPool                commandPool
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel         level
      imageCount                                      // uint32_t                     bufferCount
    };
    if (vkAllocateCommandBuffers(device, &cmdBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS) 
    {
        throw std::runtime_error("Could not Allocate Command Buffers");
    }

    std::vector<VkImage> swapchainImages(imageCount);
    if (vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not get swapchain images");
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = 
    {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // VkStructureType                        sType
      nullptr,                                      // const void                            *pNext
      VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, // VkCommandBufferUsageFlags              flags
      nullptr                                       // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
    };

    VkClearColorValue clearColor = 
    {
      { 1.0f, 0.8f, 0.4f, 0.0f }
    };

    VkImageSubresourceRange imageSubresourceRange = 
    {
      VK_IMAGE_ASPECT_COLOR_BIT,                    // VkImageAspectFlags                     aspectMask
      0,                                            // uint32_t                               baseMipLevel
      1,                                            // uint32_t                               levelCount
      0,                                            // uint32_t                               baseArrayLayer
      1                                             // uint32_t                               layerCount
    };

    for (uint32_t i = 0; i < imageCount; ++i) 
    {
        VkImageMemoryBarrier barrierFromPresentToClear = {
          VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType                        sType
          nullptr,                                    // const void                            *pNext
          VK_ACCESS_MEMORY_READ_BIT,                  // VkAccessFlags                          srcAccessMask
          VK_ACCESS_TRANSFER_WRITE_BIT,               // VkAccessFlags                          dstAccessMask
          VK_IMAGE_LAYOUT_UNDEFINED,                  // VkImageLayout                          oldLayout
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       // VkImageLayout                          newLayout
          suitableQueueFamilyIndex,                   // uint32_t                               srcQueueFamilyIndex
          suitableQueueFamilyIndex,                   // uint32_t                               dstQueueFamilyIndex
          swapchainImages[i],                         // VkImage                                image
          imageSubresourceRange                       // VkImageSubresourceRange                subresourceRange
        };

        VkImageMemoryBarrier barrierFromClearToPresent = {
          VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType                        sType
          nullptr,                                    // const void                            *pNext
          VK_ACCESS_TRANSFER_WRITE_BIT,               // VkAccessFlags                          srcAccessMask
          VK_ACCESS_MEMORY_READ_BIT,                  // VkAccessFlags                          dstAccessMask
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       // VkImageLayout                          oldLayout
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // VkImageLayout                          newLayout
          suitableQueueFamilyIndex,             // uint32_t                               srcQueueFamilyIndex
          suitableQueueFamilyIndex,             // uint32_t                               dstQueueFamilyIndex
          swapchainImages[i],                       // VkImage                                image
          imageSubresourceRange                     // VkImageSubresourceRange                subresourceRange
        };

        vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToClear);

        vkCmdClearColorImage(commandBuffers[i], swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageSubresourceRange);

        vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromClearToPresent);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error while recording the command buffer");
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
      &commandBuffers[imageIndex],                  // const VkCommandBuffer       *pCommandBuffers
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
      &renderingFinished,           // const VkSemaphore           *pWaitSemaphores
      1,                                            // uint32_t                     swapchainCount
      &swapchain,                            // const VkSwapchainKHR        *pSwapchains
      &imageIndex,                                 // const uint32_t              *pImageIndices
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
      CreateCommandBuffers();
    }
    catch(std::exception& e) 
    {
      std::cout << e.what() << std::endl;
    }
}

void initWindow() 
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "ApiWithoutSecrets_Part2", nullptr, nullptr);
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