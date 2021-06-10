/*
* Vulkan utilities
*
* Copyright(C) 2018 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include "vulkan/vulkan.h"
#include "VulkanDevice.hpp"
#if defined(__ANDROID__)
#include <android/asset_manager.h>
#elif defined(__linux__)
#include <dirent.h>
#endif
#include <RapidVulkan/Check.hpp>

namespace vkbuffer
{
	/*
		Vulkan buffer object
	*/
	struct Buffer {
		VkDevice device;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo descriptor;
		int32_t count = 0;
		void* mapped = nullptr;
		void create(vks::VulkanDevice* device, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, bool map = true) {
			this->device = device->logicalDevice;
			device->createBuffer(usageFlags, memoryPropertyFlags, size, &buffer, &memory);
			descriptor = { buffer, 0, size };
			if (map) {
				RapidVulkan::CheckError(vkMapMemory(device->logicalDevice, memory, 0, size, 0, &mapped));
			}
		}
		void destroy() {
			if (mapped) {
				unmap();
			}
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, memory, nullptr);
			buffer = VK_NULL_HANDLE;
			memory = VK_NULL_HANDLE;
		}
		void map() {
			RapidVulkan::CheckError(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped));
		}
		void unmap() {
			if (mapped) {
				vkUnmapMemory(device, memory);
				mapped = nullptr;
			}
		}
		void flush(VkDeviceSize size = VK_WHOLE_SIZE) {
			VkMappedMemoryRange mappedRange{};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = memory;
			mappedRange.size = size;
			RapidVulkan::CheckError(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
		}
	};
}