#pragma once

#include <string>
#include<vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace BadgerSandbox
{
	void WindowInit(GLFWwindow* window);
	std::vector<std::string> WindowGetPresentationExtensions();
	bool WindowShouldClose(GLFWwindow* window);
}