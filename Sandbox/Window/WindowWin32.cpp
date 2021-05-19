#include "WindowWin32.hpp"
#include <iostream>

namespace BadgerSandbox
{
  namespace
  {

  }

  WindowWin32::WindowWin32(const std::array<uint32_t, 2>& _windowSize, const std::array<uint32_t, 2>& _windowPosition)
    : windowSize(_windowSize)
	, windowPosition(_windowPosition)
  {
	  std::cout << "Here I will create a Window for windows" << std::endl;
  }

   WindowWin32::~WindowWin32()
   {
	   
   }
   std::array<uint32_t, 2> WindowWin32::GetWindowSize()
   {
	   return std::array<uint32_t, 2>();
   }
   std::array<uint32_t, 2> WindowWin32::GetWindowPosition()
   {
	   return std::array<uint32_t, 2>();
   }
}
