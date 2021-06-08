#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include<vector>

namespace BadgerSandbox
{
  void InitWindow();
  void InitWindow(GLFWwindow* window, std::vector<std::string>& presentationExtensions);
  bool ShouldClose(GLFWwindow* window);
}