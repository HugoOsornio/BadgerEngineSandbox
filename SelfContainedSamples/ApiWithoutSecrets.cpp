#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

GLFWwindow* window;
VkInstance instance;

std::vector<const char*> getRequiredExtensions() 
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}

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

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void CreateLogicalDevice()
{
  //Before creating a logical device, we need to get the number of vulkan capable physical devices in our system
  uint32_t numDevices = 0;
  if ((vkEnumeratePhysicalDevices(instance, &numDevices, nullptr) != VK_SUCCESS) ||
      (numDevices == 0))
  {
    throw std::runtime_error("Couldn't get a Physical Device");
  }
  // Once we get the number, store the devices within a structure
  std::vector<VkPhysicalDevice> physicalDevices(numDevices);
  if (vkEnumeratePhysicalDevices(instance, &numDevices, physicalDevices.data()) != VK_SUCCESS) 
  {
    throw std::runtime_error("Error occurred during physical devices enumeration!");
  }

  // Now that we have a list of physical devices (note: we may have a single device), we can check
  // their properties and their features; this sample application won't do anything fancy, so we will only query
  // but we won't do anything with the data. 

  std::vector<VkPhysicalDeviceProperties> deviceProperties{};
  std::vector<VkPhysicalDeviceFeatures> deviceFeatures{};
  deviceProperties.resize(physicalDevices.size());
  deviceFeatures.resize(physicalDevices.size());
  for (uint32_t i = 0; i < physicalDevices.size(); i++)
  {
     vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties[i]);
     /*
       Device Features
       Features are additional hardware capabilities that are similar to extensions. 
       They may not necessarily be supported by the driver and by default are not enabled.
       Features contain items such as geometry and tessellation shaders, multiple viewports, logical operations,
       or additional texture compression formats. 
       If a given physical device supports any feature we can enable it during logical device creation.
       Features are not enabled by default in Vulkan. 
       But the Vulkan spec points out that some features may have performance impact (like robustness).
     */
     vkGetPhysicalDeviceFeatures(physicalDevices[i], &deviceFeatures[i]);
  }

  /*
    Queues, Queue Families, and Command Buffers
    When we want to process any data (that is, draw a 3D scene from vertex data and vertex attributes)
    we call Vulkan functions that are passed to the driver. These functions are not passed directly,
    as sending each request separately down through a communication bus is inefficient.
    It is better to aggregate them and pass in groups. 
    In OpenGL this was done automatically by the driver and was hidden from the user.
    OpenGL API calls were queued in a buffer and if this buffer was full (or if we requested to flush it)
    whole buffer was passed to hardware for processing. 
    In Vulkan this mechanism is directly visible to the user and, more importantly,
    the user must specifically create and manage buffers for commands. 
    These are called (conveniently) command buffers.

    Command buffers (as whole objects) are passed to the hardware for execution through queues. 
    However, these buffers may contain different types of operations, such as graphics commands
    (used for generating and displaying images like in typical 3D games)
    or compute commands (used for processing data). 
    Specific types of commands may be processed by dedicated hardware,
    and that’s why queues are also divided into different types. 
    In Vulkan these queue types are called families. Each queue family may support different types of operations. 
    That’s why we also have to check if a given physical device supports the type of operations we want to perform. 
    We can also perform one type of operation on one device and another type of operation on another device, but we have to check if we can.
   */
  uint32_t suitablePhysicalDeviceIndex = 0xFFFFFFFF;
  uint32_t suitableQueueFamilyIndex = 0xFFFFFFFF;
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
          if ((queueFamilyProperties[i].queueCount > 0) &&
              (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) 
          {
              suitablePhysicalDeviceIndex = i;
              suitableQueueFamilyIndex = j;
              break;
          }
      }
  }
  
  if (suitablePhysicalDeviceIndex == 0xFFFFFFFF || suitableQueueFamilyIndex == 0xFFFFFFFF)
  {
    // Todo: Throw an error as we didn't find a suitable device nor queue family
  }
  
}

void initVulkan() 
{
    CreateInstance();
    CreateLogicalDevice();
}

void initWindow() 
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(1920, 1080, "Vulkan", nullptr, nullptr);
    //glfwSetWindowUserPointer(window, this);
    //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

int main() 
{
  std::cout << "Hello World!" << std::endl;
  initWindow();
  initVulkan();

}