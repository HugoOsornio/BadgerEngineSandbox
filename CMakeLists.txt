# 2021: Render Badger: Badger Engine Tests
# Butchered from vulkan_samples CmakeLists.txt
#

cmake_minimum_required(VERSION 3.10)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(BadgerEngineSandbox)

# create output folder
file(MAKE_DIRECTORY output)

# Add third party libraries
add_subdirectory(ThirdParty)
