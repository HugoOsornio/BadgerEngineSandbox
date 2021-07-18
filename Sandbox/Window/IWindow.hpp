#pragma once

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

namespace BadgerSandbox
{
  class IWindow
  {
    public:
    IWindow() {}
    IWindow(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition, const std::string& _windowName) {}
    ~IWindow() = default;
    virtual std::array<uint32_t, 2> GetWindowSize() = 0;
    virtual std::array<uint32_t, 2> GetWindowPosition() = 0;
    virtual std::vector<std::string> GetWindowExtensions() = 0;
    virtual void GetVulkanSurfaceFromWindow(VkInstance instance, VkSurfaceKHR* surface) = 0;
    virtual bool ShouldWindowClose() = 0;
  };
}