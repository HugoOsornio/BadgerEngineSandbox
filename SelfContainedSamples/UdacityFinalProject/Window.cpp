#include "Window.hpp"

void InitWindow(GLFWwindow* window, std::vector<std::string>& presentationExtensions)
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(1920, 1080, "UdacityFinalProject", nullptr, nullptr);
  presentationExtensions = GetPresentationExtensions();
}

std::vector<std::string> GetPresentationExtensions()
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<std::string> localExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
  return localExtensions;
}

bool ShouldClose(GLFWwindow* window)
{
  return glfwWindowShouldClose(window);
}

