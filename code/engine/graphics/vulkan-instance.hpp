#pragma once

#include <array>
#include <vector>
#include <stdexcept>

#include <vulkan/vulkan.h>

namespace vc::engine::graphics {

namespace constants {

constexpr std::array validation_layers{
	"VK_LAYER_KHRONOS_validation"
};

constexpr std::array device_extensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

} // namespace constants

class vulkan_instance {
public:
	vulkan_instance();
	~vulkan_instance();

	vulkan_instance(const vulkan_instance&) = delete;
	vulkan_instance &operator=(const vulkan_instance&) = delete;

	[[nodiscard]] auto handle() const noexcept { return m_instance; };

private:
	VkInstance m_instance{ VK_NULL_HANDLE };

#if defined(VC_DEBUG)
	VkDebugUtilsMessengerEXT m_debug_messenger{ VK_NULL_HANDLE };

	VkDebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
	void construct_debug_messenger();
#endif // defined(VC_DEBUG)

	void construct_instance();

	bool check_validation_layer_support() const;
	auto required_extensions() const -> std::vector<const char *>;
	void has_glfw_required_instance_extensions() const;
};

class vulkan_instance_error : public std::runtime_error {
public:
	using base_type = std::runtime_error;
	using base_type::runtime_error;
};

} // namespace vc::engine::graphics
