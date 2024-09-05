#include <ranges>
#include <algorithm>
#include <unordered_set>

#include <fmt/core.h>

#include "engine/graphics/device.hpp"
#include "engine/graphics/vulkan-instance.hpp"

namespace vc::engine::graphics {

bool swap_chain_support_details::is_adequate() const noexcept {
	return !(std::empty(formats) || std::empty(present_modes));
}

bool queue_family_indices::is_complete() const noexcept {
	return graphics_family.has_value() && present_family.has_value();
}

// ============================== device ============================== //
#pragma region device_implementation

device::device(vulkan_instance &instance, core::window &window) : m_instance{ instance } {
	construct_surface(window);
	select_physical_device();
	construct_logical_device();
	construct_command_pool();
}

device::~device() {
	vkDestroyCommandPool(m_device, m_command_pool, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance.handle(), m_surface, nullptr);
}

swap_chain_support_details device::query_swap_chain_support() {
	return query_swap_chain_support(m_physical_device);
}

u32 device::find_memory_type(const u32 filter, const VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);

	static constexpr auto match{ [](const auto id, const auto filter) {
		return static_cast<b8>(filter & (1 << id));
	} };
	static constexpr auto has_property{ [](const auto &memory_properties,
		const auto id, const auto properties) {
		return (memory_properties.memoryTypes[id].propertyFlags & properties) == properties;
	} };

	for (u32 i{}; i < memory_properties.memoryTypeCount; ++i) {
		if (match(i, filter) && has_property(memory_properties, i, properties)) {
			return i;
		}
	}

	throw device_error{ fmt::format(
		"Failed to find suitable memory type with {:X} filter and {:X} properties.",
		filter, properties
	) };
}

queue_family_indices device::find_queue_families() {
	return find_queue_families(m_physical_device);
}

VkFormat device::find_supported_format(const std::vector<VkFormat> &candidates,
	VkImageTiling tiling, VkFormatFeatureFlags features
) {
	VkFormatProperties props;
	const VkFormatFeatureFlags *selected_features{ nullptr };
	switch (tiling) {
		case VK_IMAGE_TILING_OPTIMAL:
			selected_features = &props.optimalTilingFeatures;
			break;
		case VK_IMAGE_TILING_LINEAR:
			selected_features = &props.linearTilingFeatures;
			break;

		default:
			throw device_error{ "Unsupported tiling format." };
	}

	for (const auto format : candidates) {
		vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);
		if ((*selected_features & features) == features) {
			return format;
		}
	}
	throw device_error{ "Failed to find supported format." };
}

VkBuffer device::make_buffer(
	const VkDeviceSize size, const VkBufferUsageFlags usage,
	const VkMemoryPropertyFlags properties, VkDeviceMemory &buffer_memory
) {
	const VkBufferCreateInfo buffer_info{
		.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size        = size,
		.usage       = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkBuffer buffer{ VK_NULL_HANDLE };
	if (VK_SUCCESS != vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer)) {
		throw device_error{
			fmt::format("Failed to create a buffer {} bytes long with the {} usage.",
				size, usage)
		};
	}

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &requirements);

	VkMemoryAllocateInfo allocate_info{
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize  = requirements.size,
		.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, properties)
	};
	if (VK_SUCCESS != vkAllocateMemory(m_device, &allocate_info, nullptr, &buffer_memory)) {
		throw device_error{
			fmt::format("Failed to allocate {} bytes for the given buffer.", requirements.size)
		};
	}

	vkBindBufferMemory(m_device, buffer, buffer_memory, 0);
	return buffer;
}


VkCommandBuffer device::begin_single_time_commands() {
	const VkCommandBufferAllocateInfo allocate_info{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = m_command_pool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(m_device, &allocate_info, &command_buffer);

	const VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(command_buffer, &begin_info);
	return command_buffer;
}

void device::end_single_time_commands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	const VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer
	};
	vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphics_queue);

	vkFreeCommandBuffers(m_device, m_command_pool, 1, &command_buffer);
}

void device::copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size) {
	auto command_buffer{ begin_single_time_commands() };

	const VkBufferCopy copy_region{ .size = size };
	vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

	end_single_time_commands(command_buffer);
}

void device::copy_buffer_to_image(VkBuffer source, VkImage image, glm::u32vec2 size, u32 layers) {
	auto command_buffer{ begin_single_time_commands() };

	const VkBufferImageCopy region{
		.imageSubresource = VkImageSubresourceLayers{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel       = 0,
			.baseArrayLayer = 0,
			.layerCount     = layers
		},
		.imageExtent = VkExtent3D{ .width = size.x, .height = size.y, .depth = 1 }
	};
	vkCmdCopyBufferToImage(command_buffer, source, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(command_buffer);
}

void device::wait_for_idle() const noexcept {
	vkDeviceWaitIdle(m_device);
}

VkImage device::make_image(const VkImageCreateInfo &info, VkMemoryPropertyFlags properties, VkDeviceMemory &image_memory) {
	VkImage image;
	if (VK_SUCCESS != vkCreateImage(m_device, &info, nullptr, &image)) {
		throw device_error{ "Cannot create image." };
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(m_device, image, &memory_requirements);

	const VkMemoryAllocateInfo allocate_info{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties)
	};

	if (VK_SUCCESS != vkAllocateMemory(m_device, &allocate_info, nullptr, &image_memory)) {
		throw device_error{
			fmt::format("Failed to allocate {} bytes for image", memory_requirements.size)
		};
	}

	if (VK_SUCCESS != vkBindImageMemory(m_device, image, image_memory, 0)) {
		throw device_error{ "Failed to bind image memory." };
	}
	return image;
}

#pragma region construct methods

void device::construct_surface(core::window &window) {
	if (m_surface = window.make_surface(m_instance); m_surface == VK_NULL_HANDLE) {
		throw device_error{ "Failed to create the surface." };
	}
}

void device::select_physical_device() {
	u32 device_count{};
	vkEnumeratePhysicalDevices(m_instance.handle(), &device_count, nullptr);
	if (device_count == 0) {
		throw device_error{ "Failed to find Vulkan-supported GPUs" };
	}
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance.handle(), &device_count, std::data(devices));

	const auto suitable{ [this] (const auto &device) { return is_suitable(device); } };
	if (auto found{ std::ranges::find_if(devices, suitable) }; found != std::end(devices)) {
		m_physical_device = *found;
		vkGetPhysicalDeviceProperties(m_physical_device, &m_physical_device_properties);
		std::printf("[ending][graphics][device] Selected device: %s\n",
			m_physical_device_properties.deviceName);
		return;
	}
	throw device_error{ fmt::format("No suitable device from {} was found", device_count) };
}

void device::construct_logical_device() {
	static constexpr auto make_queue_info{ [](auto index, auto *priorities) {
		return VkDeviceQueueCreateInfo{
			.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = index,
			.queueCount       = 1,
			.pQueuePriorities = priorities
		};
	} };

	const auto indices{ find_queue_families() };
	if (!indices.is_complete()) {
		throw device_error{ "Could not find graphics and present queue families." };
	}

	f32 priority{ 1.0f };
	const std::vector<VkDeviceQueueCreateInfo> queue_create_infos{
		make_queue_info(indices.graphics_family.value_or(0), &priority),
		make_queue_info(indices.present_family.value_or(0), &priority)
	};

	const VkPhysicalDeviceFeatures features{
		.samplerAnisotropy = VK_TRUE
	};

	const VkDeviceCreateInfo create_info{
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount    = static_cast<u32>(std::size(queue_create_infos)),
		.pQueueCreateInfos       = std::data(queue_create_infos),
		.enabledExtensionCount   = static_cast<u32>(std::size(constants::device_extensions)),
		.ppEnabledExtensionNames = std::data(constants::device_extensions),
		.pEnabledFeatures        = &features
	};
	if (VK_SUCCESS != vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device)) {
		throw device_error{ fmt::format(
			"Failed to create logical device from {} physical device.",
				m_physical_device_properties.deviceName
		) };
	}

	vkGetDeviceQueue(m_device, indices.graphics_family.value_or(0), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, indices.present_family.value_or(0), 0, &m_present_queue);
}

void device::construct_command_pool() {
	const auto family_indices{ find_queue_families() };

	const VkCommandPoolCreateInfo pool_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags
			= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
			| VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = family_indices.graphics_family.value_or(0)
	};
	if (VK_SUCCESS != vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool)) {
		throw device_error{ "Failed to create command pool." };
	}
}

#pragma endregion construct methods


bool device::is_suitable(VkPhysicalDevice device) {
	if (!check_device_extension_support(device)) return false;
	if (const auto indices{ find_queue_families(device) }; !indices.is_complete()) return false;
	if (const auto support{ query_swap_chain_support(device) }; !support.is_adequate()) return false;

	VkPhysicalDeviceFeatures features{};
	vkGetPhysicalDeviceFeatures(device, &features);
	return features.samplerAnisotropy;
}

bool device::check_device_extension_support(VkPhysicalDevice device) {
	u32 extensions_count{};
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);

	std::vector<VkExtensionProperties> properties(extensions_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, std::data(properties));

	const auto supported_by_device{ [&properties] (const std::string_view extension) {
		for (const auto &property : properties) {
			if (extension == property.extensionName) return true;
		}
		return false;
	} };
	return std::ranges::all_of(constants::device_extensions, supported_by_device);
}

queue_family_indices device::find_queue_families(VkPhysicalDevice device) {
	u32 queue_family_count{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
		std::data(queue_families));

	queue_family_indices indices;
	for (u32 family_id{}; family_id < queue_family_count && !indices.is_complete(); ++family_id) {
		const auto &family{ queue_families[family_id] };
		if (family.queueCount == 0) continue;

		if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = family_id;
			continue;
		}
		VkBool32 present_support{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(device, family_id, m_surface, &present_support);
		if (present_support == VK_TRUE) {
			indices.present_family = family_id;
		}
	}
	return indices;
}

swap_chain_support_details device::query_swap_chain_support(VkPhysicalDevice device) {
	swap_chain_support_details details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	u32 format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count,
			std::data(details.formats));
	}

	u32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count,
			std::data(details.present_modes));
	}

	return details;
}

#pragma endregion device_implementation

} // namespace vc::engine::graphics

