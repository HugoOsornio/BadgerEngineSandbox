#include <iostream>
#include <array>
#include "WindowFactory.hpp"

int main()
{
    std::cout << "Hello World!" << std::endl;
    BadgerSandbox::WindowFactory windowFactory;
    windowFactory.Allocate(std::array<uint32_t, 2>{1280, 720}, std::array<uint32_t, 2>{0, 0});
    return 0;
}
