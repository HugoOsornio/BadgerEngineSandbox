#include "WindowFactory.hpp"

#if defined(_WIN32)
#include "WindowWin32.hpp"
namespace BadgerSandbox
{
  using Window = WindowWin32;
}
#elif defined(__linux__)
#include "WindowLinux.hpp"
namespace BadgerSandbox
{
  using Window = WindowLinux;
}
#else
#error Unsupported window platform detected!
#endif

namespace BadgerSandbox
{
  std::shared_ptr<IWindow> WindowFactory::Create(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition, const std::string& _windowName)
  {
    return std::make_shared<Window>(_windowSize, _windowPosition, _windowName);
  }
}
