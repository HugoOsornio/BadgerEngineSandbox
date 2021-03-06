# 2021: Render Badger: Badger Engine Tests
# Butchered from vulkan_samples and Sascha Willems CmakeLists.txt
#

cmake_minimum_required(VERSION 3.10)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(BadgerEngineSandbox)

# Add third party libraries
add_subdirectory(ThirdParty)

# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)
endif()

# Add VUlkan to the project
IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	find_package(Threads REQUIRED)
       IF(USE_WAYLAND_WSI)
		find_program(PKG_CONFIG pkg-config)
		if (NOT PKG_CONFIG)
			message(FATAL_ERROR "pkg-config binary not found")
		endif ()
		find_package(Wayland REQUIRED)
		if (NOT WAYLAND_FOUND)
			message(FATAL_ERROR "Wayland development package not found")
		endif ()
		pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
		if (NOT WAYLAND_PROTOCOLS_FOUND)
			message(FATAL_ERROR "Wayland protocols package not found")
		endif ()
		find_program(WAYLAND_SCANNER wayland-scanner)
		if (NOT WAYLAND_SCANNER)
			message(FATAL_ERROR "wayland-scanner binary not found")
		endif ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
		include_directories(${WAYLAND_INCLUDE_DIR})
		execute_process(COMMAND ${PKG_CONFIG} --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE protocol_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
				COMMAND ${WAYLAND_SCANNER} private-code ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
		include_directories(${CMAKE_BINARY_DIR})
	ELSE(USE_WAYLAND_WSI)
	    # We default to XCB on Linux, unless Wayland is directly requested.
		find_package(XCB REQUIRED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
	ENDIF(USE_D2D_WSI)
ENDIF(WIN32)

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

include_directories(${Vulkan_INCLUDE_DIRS})
add_executable(ApiWithoutSecrets SelfContainedSamples/ApiWithoutSecrets.cpp)
target_link_libraries(ApiWithoutSecrets ${Vulkan_LIBRARY} glfw)

add_executable(ApiWithoutSecrets_Part2 SelfContainedSamples/ApiWithoutSecrets_Part2.cpp)
target_link_libraries(ApiWithoutSecrets_Part2 ${Vulkan_LIBRARY} glfw)

add_executable(ApiWithoutSecrets_Part3 SelfContainedSamples/ApiWithoutSecrets_Part3.cpp)
target_compile_definitions(ApiWithoutSecrets_Part3 PUBLIC -DAPI_WITHOUT_SECRETS_PART3_CONTENT="${CMAKE_SOURCE_DIR}/SelfContainedSamples/ApiWithoutSecrets_Part3Content/")
target_link_libraries(ApiWithoutSecrets_Part3 ${Vulkan_LIBRARY} glfw)

add_executable(ApiWithoutSecrets_Part4 SelfContainedSamples/ApiWithoutSecrets_Part4.cpp)
target_compile_definitions(ApiWithoutSecrets_Part4 PUBLIC -DAPI_WITHOUT_SECRETS_PART4_CONTENT="${CMAKE_SOURCE_DIR}/SelfContainedSamples/ApiWithoutSecrets_Part4Content/")
target_link_libraries(ApiWithoutSecrets_Part4 ${Vulkan_LIBRARY} glfw)

add_executable(ApiWithoutSecrets_Part5 SelfContainedSamples/ApiWithoutSecrets_Part5.cpp)
target_compile_definitions(ApiWithoutSecrets_Part5 PUBLIC -DAPI_WITHOUT_SECRETS_PART5_CONTENT="${CMAKE_SOURCE_DIR}/SelfContainedSamples/ApiWithoutSecrets_Part5Content/")
target_link_libraries(ApiWithoutSecrets_Part5 ${Vulkan_LIBRARY} glfw)

add_executable(ApiWithoutSecrets_Part6 SelfContainedSamples/ApiWithoutSecrets_Part6.cpp SelfContainedSamples/RenderBadger.cpp)
target_compile_definitions(ApiWithoutSecrets_Part6 PUBLIC -DAPI_WITHOUT_SECRETS_PART6_CONTENT="${CMAKE_SOURCE_DIR}/SelfContainedSamples/ApiWithoutSecrets_Part6Content/")
target_link_libraries(ApiWithoutSecrets_Part6 ${Vulkan_LIBRARY} glfw)

add_executable(UdacityFinalProject SelfContainedSamples/UdacityFinalProject/UdacityFinalProject.cpp SelfContainedSamples/UdacityFinalProject/Window.cpp SelfContainedSamples/UdacityFinalProject/VulkanglTFModel.hpp SelfContainedSamples/UdacityFinalProject/VulkanDevice.hpp SelfContainedSamples/UdacityFinalProject/VulkanUtils.hpp)
target_compile_definitions(UdacityFinalProject PUBLIC -DUDACITY_FINAL_PROJECT_CONTENT="${CMAKE_SOURCE_DIR}/SelfContainedSamples/UdacityFinalProject/Content/")
target_link_libraries(UdacityFinalProject ${Vulkan_LIBRARY} glfw RapidVulkan tinygltf glm)

add_executable(VectorVulkanTest Sandbox/VectorVulkanTest/VectorVulkanTest.cpp Sandbox/Window/WindowFactory.cpp Sandbox/Window/WindowWin32.cpp Sandbox/Matrix/Matrix3D.cpp Sandbox/Matrix/Matrix4D.cpp Sandbox/Vector/Vector3D.cpp Sandbox/Vector/Vector4D.cpp)
target_include_directories(VectorVulkanTest PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Window> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Matrix> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Vector>)
target_compile_definitions(VectorVulkanTest PUBLIC -DVECTOR_TEST_PROJECT_CONTENT="${CMAKE_SOURCE_DIR}/Sandbox/VectorVulkanTest/Content/")
target_link_directories(VectorVulkanTest PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Window> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Matrix> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Vector>)
target_link_libraries(VectorVulkanTest ${Vulkan_LIBRARY} glfw RapidVulkan glm)

add_executable(PhongShading Sandbox/PhongShading/PhongShading.cpp Sandbox/PhongShading/VulkanglTFModel.hpp Sandbox/Window/WindowFactory.cpp Sandbox/Window/WindowWin32.cpp)
target_include_directories(PhongShading PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Window>)
target_compile_definitions(PhongShading PUBLIC -DPHONG_PROJECT_CONTENT="${CMAKE_SOURCE_DIR}/Sandbox/PhongShading/Content/")
target_link_libraries(PhongShading ${Vulkan_LIBRARY} glfw RapidVulkan tinygltf glm)

add_executable(ShadowMapping Sandbox/ShadowMapping/ShadowMapping.cpp Sandbox/ShadowMapping/VulkanglTFModel.hpp Sandbox/Window/WindowFactory.cpp Sandbox/Window/WindowWin32.cpp)
target_include_directories(ShadowMapping PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/Window>)
target_compile_definitions(ShadowMapping PUBLIC -DSHADOW_MAPPING_PROJECT_CONTENT="${CMAKE_SOURCE_DIR}/Sandbox/ShadowMapping/Content/")
target_link_libraries(ShadowMapping ${Vulkan_LIBRARY} glfw RapidVulkan tinygltf glm)
