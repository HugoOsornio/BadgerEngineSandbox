#pragma once

#include <string>
#include<vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace BadgerSandbox
{
	GLFWwindow* WindowInit();
	std::vector<std::string> WindowGetPresentationExtensions();
	bool WindowShouldClose(GLFWwindow* window);
}