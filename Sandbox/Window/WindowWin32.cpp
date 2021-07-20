#include "WindowWin32.hpp"
#include <iostream>

namespace BadgerSandbox
{
  namespace
  {

  }

  WindowWin32::WindowWin32(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition, const std::string& _windowName )
    : windowSize(_windowSize)
	, windowPosition(_windowPosition)
  {
	  glfwInit();
	  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	  m_nativeWindow = glfwCreateWindow(_windowSize[0], _windowSize[1], _windowName.c_str(), nullptr, nullptr);
  }

   WindowWin32::~WindowWin32()
   {
	   
   }
   std::array<uint32_t, 2> WindowWin32::GetWindowSize()
   {
	   std::array<int32_t, 2> signedWindowSize{ 0,0 };
	   glfwGetWindowSize(m_nativeWindow, &signedWindowSize[0], &signedWindowSize[1]);
	   return std::array<uint32_t, 2>{(uint32_t)signedWindowSize[0], (uint32_t)signedWindowSize[1]};
   }
   std::array<uint32_t, 2> WindowWin32::GetWindowPosition()
   {
	   return std::array<uint32_t, 2>();
   }
   
   std::vector<std::string> WindowWin32::GetWindowExtensions()
   {
	   uint32_t glfwExtensionCount = 0;
	   const char** glfwExtensions;
	   glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	   std::vector<std::string> localExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	   return localExtensions;
   }

   void WindowWin32::GetVulkanSurfaceFromWindow(VkInstance instance, VkSurfaceKHR* surface)
   {
	   if (glfwCreateWindowSurface(instance, m_nativeWindow, nullptr, surface) != VK_SUCCESS)
	   {
		   throw std::runtime_error("failed to create window surface!");
	   }
   }

   bool WindowWin32::ShouldWindowClose()
   {
	   glfwPollEvents();
	   return glfwWindowShouldClose(m_nativeWindow);
   }

   void* WindowWin32::GetNativeWindow()
   {
	   return (void*)m_nativeWindow;
   }
}
