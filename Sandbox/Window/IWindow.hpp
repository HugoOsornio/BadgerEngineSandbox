#pragma once

#include <array>
#include <memory>

namespace BadgerSandbox
{
  class IWindow
  {
    public:
    IWindow(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition) = 0;
    virtual ~IWindow() = default;

    virtual std::array<uint32_t, 2> GetWindowSize() const = 0;
	virtual std::array<uint32_t, 2> GetWindowPosition() const = 0;

    protected:
      
  };
}