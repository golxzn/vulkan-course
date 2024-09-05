#include <string>
#include <unordered_set>

#include <fmt/core.h>

#include "engine/graphics/vulkan-instance.hpp"

#include "core/glfw-header.hpp"

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

vulkan_instance::vulkan_instance() {
	construct_instance();
#if defined(VC_DEBUG)
	construct_debug_messenger();
#endif // defined(VC_DEBUG)
}

vulkan_instance::~vulkan_instance() {
#if defined(VC_DEBUG)
	destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr);
#endif // defined(VC_DEBUG)

	vkDestroyInstance(m_instance, nullptr);
}

#if defined(VC_DEBUG)

VkDebugUtilsMessengerCreateInfoEXT vulkan_instance::make_debug_messenger_create_info() {
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

void vulkan_instance::construct_debug_messenger() {
	const auto create_info{ make_debug_messenger_create_info() };
	m_debug_messenger = create_debug_utils_messenger(m_instance, &create_info, nullptr);
	if (m_debug_messenger == nullptr) {
		throw vulkan_instance_error{ "Failed to construct the debug messenger." };
	}
}

bool vulkan_instance::check_validation_layer_support() const {
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

#endif // defined(VC_DEBUG)

void vulkan_instance::construct_instance() {
#if defined(VC_DEBUG)
	if (!check_validation_layer_support()) {
		throw vulkan_instance_error{ "Validation layers are not available, but required." };
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
		throw vulkan_instance_error{ "Failed to create the vulkan instance." };
	}

	has_glfw_required_instance_extensions();
}

std::vector<const char *> vulkan_instance::required_extensions() const {
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

void vulkan_instance::has_glfw_required_instance_extensions() const {
	uint32_t extensions_count{};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);

	std::vector<VkExtensionProperties> extensions(extensions_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, std::data(extensions));

	const std::unordered_set<std::string_view> available{ [] (const auto &extensions) {
		std::unordered_set<std::string_view> result;
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
		throw vulkan_instance_error{
			fmt::format("Missing required extensions: {}", std::move(missing))
		};
	}
}


} // namespace vc::engine::graphics

