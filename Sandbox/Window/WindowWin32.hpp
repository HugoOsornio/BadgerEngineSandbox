#pragma once

#include "IWindow.hpp"
#include <memory>
#include <array>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace BadgerSandbox
{
  class WindowWin32 : virtual public IWindow
  {
  public:
	 WindowWin32::WindowWin32(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition, const std::string& _windowName);
    ~WindowWin32();
	std::array<uint32_t, 2> GetWindowSize();
	std::array<uint32_t, 2> GetWindowPosition();
	std::vector<std::string> GetWindowExtensions();
	void GetVulkanSurfaceFromWindow(VkInstance, VkSurfaceKHR* surface);
	bool ShouldWindowClose();
	void* GetNativeWindow();

	private:
	GLFWwindow* m_nativeWindow;
    std::array<uint32_t, 2> windowSize;
	std::array<uint32_t, 2> windowPosition;
  };
}