#include "WindowWin32.hpp"

namespace BadgerSandbox
{
  namespace
  {

  }

  WindowWin32::WindowWin32(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition)
    : windowSize(_windowSize)
	, windowPosition(_windowPosition)
  {
    
  }

   WindowWin32::~WindowWin32()
   {
	   
   }


  PlatformNativeWindowType VulkanNativeWindowWin32::GetWindowType() const
  {
    return GetPlatformWindow();
  }
}    // namespace Fsl
#endif
