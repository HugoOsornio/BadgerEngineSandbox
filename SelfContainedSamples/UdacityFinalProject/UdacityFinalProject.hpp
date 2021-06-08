#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Window.hpp"

namespace BadgerSandbox
{
  GLFWwindow* window;
  std::vector<std::string> presentationExtensions
  std::unordered_map<std::string, bool> requestedExtensions;
  std::vector<VkExtensionProperties> availableExtensions;
}