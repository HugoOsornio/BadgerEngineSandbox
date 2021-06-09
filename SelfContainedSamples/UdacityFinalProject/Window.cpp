#include "Window.hpp"

namespace BadgerSandbox
{
	void WindowInit(GLFWwindow* window)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(1920, 1080, "UdacityFinalProject", nullptr, nullptr);
	}

	std::vector<std::string> WindowGetPresentationExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<std::string> localExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		return localExtensions;
	}

	bool WindowShouldClose(GLFWwindow* window)
	{
		return glfwWindowShouldClose(window);
	}
}