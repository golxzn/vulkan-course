#pragma once

#include <array>
#include <vector>
#include <exception>
#include <stdexcept>

#include "core/window.hpp"

namespace vc::engine::graphics {

namespace constants {

constexpr std::array validation_layers{
	"VK_LAYER_KHRONOS_validation"
};

constexpr std::array device_extensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

} // namespace constants

struct swap_chain_support_details {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;

	[[nodiscard]] bool is_adequate() const noexcept;
};

struct queue_family_indices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	[[nodiscard]] bool is_complete() const noexcept;
};

class device_error : public std::runtime_error {
public:
	using base_type = std::runtime_error;
	using base_type::runtime_error;
};

class device {
public:
	explicit device(core::window &window);
	~device();

	device(const device &) = delete;
	device(device &&) noexcept = delete;
	device &operator=(const device &) = delete;
	device &operator=(device &&) noexcept = delete;

	[[nodiscard]] decltype(auto) command_pool() noexcept { return m_command_pool; }
	[[nodiscard]] decltype(auto) handle() noexcept { return m_device; }
	[[nodiscard]] decltype(auto) surface() noexcept { return m_surface; }
	[[nodiscard]] decltype(auto) graphics_queue() noexcept { return m_graphics_queue; }
	[[nodiscard]] decltype(auto) present_queue() noexcept { return m_present_queue; }

	[[nodiscard]] auto query_swap_chain_support() -> swap_chain_support_details;
	[[nodiscard]] auto find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties) -> uint32_t;
	[[nodiscard]] auto find_queue_families() -> queue_family_indices;
	[[nodiscard]] auto find_supported_format(const std::vector<VkFormat> &candidates,
		VkImageTiling tiling, VkFormatFeatureFlags features) -> VkFormat;

	[[nodiscard]] auto make_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties, VkDeviceMemory &buffer_memory) -> VkBuffer;

	[[nodiscard]] auto begin_single_time_commands() -> VkCommandBuffer;
	void end_single_time_commands(VkCommandBuffer command_buffer);

	void copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size);
	void copy_buffer_to_image(VkBuffer source, VkImage image, glm::u32vec2 size, uint32_t layers);

	void wait_for_idle() const noexcept;

	[[nodiscard]] auto make_image(const VkImageCreateInfo &info, VkMemoryPropertyFlags properties,
		VkDeviceMemory &image_memory) -> VkImage;

private:
	VkInstance                 m_instance                  { VK_NULL_HANDLE };
	VkPhysicalDevice           m_physical_device           { VK_NULL_HANDLE };
  	VkPhysicalDeviceProperties m_physical_device_properties{};
	VkCommandPool              m_command_pool              { VK_NULL_HANDLE };

	VkDevice     m_device        { VK_NULL_HANDLE };
	VkSurfaceKHR m_surface       { VK_NULL_HANDLE };
	VkQueue      m_graphics_queue{ VK_NULL_HANDLE };
	VkQueue      m_present_queue { VK_NULL_HANDLE };

#if defined(VC_DEBUG)
	VkDebugUtilsMessengerEXT m_debug_messenger{ VK_NULL_HANDLE };

	VkDebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
	void construct_debug_messenger();
#endif // defined(VC_DEBUG)

	void construct_instance();
	void construct_surface(core::window &window);
	void select_physical_device();
	void construct_logical_device();
	void construct_command_pool();

	bool is_suitable(VkPhysicalDevice device);
	bool check_validation_layer_support();
	bool check_device_extension_support(VkPhysicalDevice device);
	auto required_extensions() -> std::vector<const char *>;
	auto find_queue_families(VkPhysicalDevice device) -> queue_family_indices;
	auto query_swap_chain_support(VkPhysicalDevice device) -> swap_chain_support_details;

	void has_glfw_required_instance_extensions();
};

} // namespace vc::engine::graphics


