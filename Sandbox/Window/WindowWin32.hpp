#pragma once

#include "IWindow.hpp"
#include <memory>
#include <array>

namespace BadgerSandbox
{
  class WindowWin32 : virtual public IWindow
  {
  public:
	 WindowWin32(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition);
    ~WindowWin32();
	std::array<uint32_t, 2> GetWindowSize();
	std::array<uint32_t, 2> GetWindowPosition();

	private:
    std::array<uint32_t, 2> windowSize;
	std::array<uint32_t, 2> windowPosition;
  };
}