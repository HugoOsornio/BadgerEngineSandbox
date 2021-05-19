#pragma once

#include <array>
#include <memory>

namespace BadgerSandbox
{
  class IWindow
  {
    public:
    IWindow() {}
    IWindow(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition) {}
    ~IWindow() = default;
    std::array<uint32_t, 2> GetWindowSize() {};
    std::array<uint32_t, 2> GetWindowPosition() {};
  };
}