#pragma once

#include <array>
#include <limits>
#include <vector>
#include <optional>
#include <stdexcept>

#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace vc::engine::graphics {

class device;

namespace constants {

constexpr i32  max_frames_in_flight{ 2 };
constexpr u64 fence_wait_timeout  { std::numeric_limits<u64>::max() };
constexpr u64 acquire_next_timeout{ std::numeric_limits<u64>::max() };

} // namespace constants

template<class T>
using max_frame_array = std::array<T, constants::max_frames_in_flight>;

class swap_chain {
public:
	explicit swap_chain(device &device, VkExtent2D extent);
	~swap_chain();

	swap_chain(const swap_chain &) = delete;
	swap_chain &operator=(const swap_chain &) = delete;

	[[nodiscard]] auto framebuffer(const size_t index) const { return m_framebuffers.at(index); }
	[[nodiscard]] auto render_pass() const noexcept { return m_render_pass; }
	[[nodiscard]] auto image_view(const size_t index) const { return m_image_views.at(index); }
	[[nodiscard]] auto image_count() const noexcept { return std::size(m_images); }
	[[nodiscard]] auto image_format() const noexcept { return m_image_format; }
	[[nodiscard]] auto extent() const noexcept { return m_extent; }

	[[nodiscard]] auto aspect_ratio() const noexcept -> f32;
	[[nodiscard]] auto find_depth_format() const -> VkFormat;

	[[nodiscard]] auto acquire_next_image() -> std::optional<u32>;
	[[nodiscard]] auto submit(u32 image_index, const VkCommandBuffer *buffers,
		u32 buffers_count = 1) -> VkResult;

private:
	device                      &m_device;

	VkSwapchainKHR               m_swap_chain{ nullptr };

	VkFormat                     m_image_format;
	VkExtent2D                   m_extent;
	VkExtent2D                   m_window_extent;

	std::vector<VkFramebuffer>   m_framebuffers;
	VkRenderPass                 m_render_pass;

	std::vector<VkImage>         m_depth_images;
	std::vector<VkDeviceMemory>  m_depth_image_memories;
	std::vector<VkImageView>     m_depth_image_views;
	std::vector<VkImage>         m_images;
	std::vector<VkImageView>     m_image_views;

	max_frame_array<VkSemaphore> m_available_images_semaphores;
	max_frame_array<VkSemaphore> m_render_finished_semaphores;
	max_frame_array<VkFence>     m_in_flight_fences;
	std::vector<VkFence>         m_images_in_flight;

	size_t m_current_frame{};

	void construct_swap_chain();
	void construct_image_views();
	void construct_render_pass();
	void construct_depth_resources();
	void construct_framebuffers();
	void construct_sync_objects();

	auto select_surface_format(const std::vector<VkSurfaceFormatKHR> &available) -> VkSurfaceFormatKHR;
	auto select_present_mode(const std::vector<VkPresentModeKHR> &available) -> VkPresentModeKHR;
	auto select_extent(const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D;
};

class swap_chain_error : public std::runtime_error {
public:
	using base_type = std::runtime_error;
	using base_type::runtime_error;
};

} // namespace vc::engine::graphics
