#include <ranges>
#include <algorithm>
#include <unordered_set>

#include <fmt/core.h>

#include "engine/graphics/device.hpp"

namespace vc::engine::graphics {

#pragma region utilities
namespace {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *
) {
	static constexpr auto match{ [](const auto severity, const auto flag) {
		return (severity & flag) == flag;
	} };

	static constexpr auto to_str{ [](VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
		if (match(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))   return "ERR ";// = 0x00001000,
		if (match(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) return "WARN";// = 0x00000100,
		if (match(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))    return "INFO";// = 0x00000010,
		if (match(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) return "VERB";// = 0x00000001,
		return "UNKN";
	} };


	std::fprintf(stderr, "[VULKAN][%s] %s\n", to_str(severity), callback_data->pMessage);
	return VK_FALSE;
}

#if defined(VC_DEBUG)

VkDebugUtilsMessengerEXT create_debug_utils_messenger(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *create_info, const VkAllocationCallbacks *allocator) {
	const auto constructor{ reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
	) };

	if (constructor == nullptr) return nullptr;

	if (VkDebugUtilsMessengerEXT messenger;
		VK_SUCCESS == constructor(instance, create_info, allocator, &messenger)) {
		return messenger;
	}
	return nullptr;
}

void destroy_debug_utils_messenger(VkInstance instance,
	VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *allocator) {
	const auto destructor{ reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
	) };

	if (destructor != nullptr) {
		destructor(instance, messenger, allocator);
	}
}


#endif // defined(VC_DEBUG)

} // anonymous namespace
#pragma endregion utilities


bool swap_chain_support_details::is_adequate() const noexcept {
	return !(std::empty(formats) || std::empty(present_modes));
}

bool queue_family_indices::is_complete() const noexcept {
	return graphics_family.has_value() && present_family.has_value();
}

// ============================== device ============================== //
#pragma region device_implementation

device::device(core::window &window) {
	construct_instance();
#if defined(VC_DEBUG)
	construct_debug_messenger();
#endif // defined(VC_DEBUG)
	construct_surface(window);
	select_physical_device();
	construct_logical_device();
	construct_command_pool();
}

device::~device() {
	vkDestroyCommandPool(m_device, m_command_pool, nullptr);
	vkDestroyDevice(m_device, nullptr);

#if defined(VC_DEBUG)
	destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr);
#endif // defined(VC_DEBUG)

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

swap_chain_support_details device::query_swap_chain_support() {
	return query_swap_chain_support(m_physical_device);
}

uint32_t device::find_memory_type(const uint32_t filter, const VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);
	for (uint32_t i{}; i < memory_properties.memoryTypeCount; ++i) {
		if ((filter & (1 << i)) && ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)) {
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

VkFormat device::find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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

void device::copy_buffer_to_image(VkBuffer source, VkImage image, glm::u32vec2 size, uint32_t layers) {
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


#if defined(VC_DEBUG)

VkDebugUtilsMessengerCreateInfoEXT device::make_debug_messenger_create_info() {
	return VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity
			= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType
			= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debug_callback,
		.pUserData       = nullptr
	};
}

#endif // defined(VC_DEBUG)

#pragma region construct methods

#if defined(VC_DEBUG)

void device::construct_debug_messenger() {
	const auto create_info{ make_debug_messenger_create_info() };
	m_debug_messenger = create_debug_utils_messenger(m_instance, &create_info, nullptr);
	if (m_debug_messenger == nullptr) {
		throw device_error{ "Failed to construct the debug messenger." };
	}
}

#endif // defined(VC_DEBUG)

void device::construct_instance() {
#if defined(VC_DEBUG)
	if (!check_validation_layer_support()) {
		throw device_error{ "Validation layers are not available, but required." };
	}
	const auto debug_create_info{ make_debug_messenger_create_info() };
#endif // defined(VC_DEBUG)

	const VkApplicationInfo application_info{
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = "vulkan-course-application",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName        = "vulkan-course-engine",
		.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion         = VK_API_VERSION_1_0
	};

	const auto extensions{ required_extensions() };
	const VkInstanceCreateInfo create_info{
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if defined(VC_DEBUG)
		.pNext                   = &debug_create_info,
		.pApplicationInfo        = &application_info,
		.enabledLayerCount       = static_cast<uint32_t>(std::size(constants::validation_layers)),
		.ppEnabledLayerNames     = std::data(constants::validation_layers),
#else
		.pApplicationInfo        = &application_info,
#endif // defined(VC_DEBUG)
		.enabledExtensionCount   = static_cast<uint32_t>(std::size(extensions)),
		.ppEnabledExtensionNames = std::data(extensions)
	};

	if (VK_SUCCESS != vkCreateInstance(&create_info, nullptr, &m_instance)) {
		throw device_error{ "Failed to create the vulkan instance." };
	}

	has_glfw_required_instance_extensions();
}

void device::construct_surface(core::window &window) {
	if (m_surface = window.make_surface(m_instance); m_surface == VK_NULL_HANDLE) {
		throw device_error{ "Failed to create the surface." };
	}
}

void device::select_physical_device() {
	uint32_t device_count{};
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
	if (device_count == 0) {
		throw device_error{ "Failed to find Vulkan-supported GPUs" };
	}
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, std::data(devices));

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

	float priority{ 1.0f };
	const std::vector<VkDeviceQueueCreateInfo> queue_create_infos{
		make_queue_info(indices.graphics_family.value_or(0), &priority),
		make_queue_info(indices.present_family.value_or(0), &priority)
	};

	const VkPhysicalDeviceFeatures features{
		.samplerAnisotropy = VK_TRUE
	};

	const VkDeviceCreateInfo create_info{
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount    = static_cast<uint32_t>(std::size(queue_create_infos)),
		.pQueueCreateInfos       = std::data(queue_create_infos),
		.enabledExtensionCount   = static_cast<uint32_t>(std::size(constants::device_extensions)),
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

bool device::check_validation_layer_support() {
	uint32_t layers_count{};
	vkEnumerateInstanceLayerProperties(&layers_count, nullptr);

	std::vector<VkLayerProperties> properties(layers_count);
	vkEnumerateInstanceLayerProperties(&layers_count, std::data(properties));

	for (const std::string_view layer_name : constants::validation_layers) {
		bool found{ false };
		for (const auto &property : properties) {
			if (layer_name == property.layerName) {
				found = true;
				break;
			}
		}

		if (!found) return false;
	}

	return true;
}

bool device::check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extensions_count{};
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

std::vector<const char *> device::required_extensions() {
	uint32_t glfw_extensions_count{};
	const char **glfw_extensions{ glfwGetRequiredInstanceExtensions(&glfw_extensions_count) };

#if defined(VC_DEBUG)
	std::vector<const char *> extensions(glfw_extensions_count + 1);
	std::copy_n(glfw_extensions, glfw_extensions_count, std::data(extensions));
	extensions.back() = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	return extensions;
#else
	return std::vector<const char *>(glfw_extensions, glfw_extensions + glfw_extensions_count);
#endif // defined(VC_DEBUG)
}

queue_family_indices device::find_queue_families(VkPhysicalDevice device) {
	uint32_t queue_family_count{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
		std::data(queue_families));

	queue_family_indices indices;
	for (uint32_t family_id{}; family_id < queue_family_count && !indices.is_complete(); ++family_id) {
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

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count,
			std::data(details.formats));
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count,
			std::data(details.present_modes));
	}

	return details;
}

void device::has_glfw_required_instance_extensions() {
	uint32_t extensions_count{};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);

	std::vector<VkExtensionProperties> extensions(extensions_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, std::data(extensions));

	const std::unordered_set<std::string> available{ [] (const auto &extensions) {
		std::unordered_set<std::string> result;
		for (const auto &extension : extensions) {
			result.emplace(extension.extensionName);
		}
		return result;
	}(extensions) };

	const auto required{ required_extensions() };
	std::string missing;
	missing.reserve(200); // 200 to prevent a lot of reallocations
	for (const auto &extension : required) {
		if (!available.contains(extension)) {
			missing += extension;
			missing += ", ";
		}
	}
	if (!std::empty(missing)) {
		missing.resize(std::size(missing) - std::size(", "));
		throw device_error{ fmt::format("Missing required extensions: {}", std::move(missing)) };
	}
}

#pragma endregion device_implementation

} // namespace vc::engine::graphics

