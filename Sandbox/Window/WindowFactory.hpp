#pragma once

#include "IWindow.hpp"
#include <array>

namespace BadgerSandbox
{
  class WindowFactory
  {
  public:
    static std::shared_ptr<IWindow> Allocate(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition);
  };
}
