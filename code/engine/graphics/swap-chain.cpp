#include <algorithm>

#include "engine/graphics/swap-chain.hpp"
#include "engine/graphics/device.hpp"

namespace vc::engine::graphics {

swap_chain::swap_chain(device &_device, const VkExtent2D extent)
	: m_device{ _device }, m_window_extent{ extent } {
	construct_swap_chain();
	construct_image_views();
	construct_render_pass();
	construct_depth_resources();
	construct_framebuffers();
	construct_sync_objects();
}

swap_chain::~swap_chain() {
	auto device{ m_device.handle() };

	for (auto &&image_view : m_image_views) {
		vkDestroyImageView(device, image_view, nullptr);
	}

	if (m_swap_chain != nullptr) {
		vkDestroySwapchainKHR(device, std::exchange(m_swap_chain, nullptr), nullptr);
	}

	for (size_t i{}; i < std::size(m_depth_images); ++i) {
		vkDestroyImageView(device, m_depth_image_views[i], nullptr);
		vkDestroyImage(device, m_depth_images[i], nullptr);
		vkFreeMemory(device, m_depth_image_memories[i], nullptr);
	}

	for (auto &&framebuffer : m_framebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(device, m_render_pass, nullptr);

	for (size_t i{}; i < std::size(m_render_finished_semaphores); ++i) {
		vkDestroySemaphore(device, m_render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, m_available_images_semaphores[i], nullptr);
		vkDestroyFence(device, m_in_flight_fences[i], nullptr);
	}
}

float swap_chain::aspect_ratio() const noexcept {
	return static_cast<float>(m_extent.width) / static_cast<float>(m_extent.height);
}

VkFormat swap_chain::find_depth_format() const {
	return m_device.find_supported_format(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

std::optional<uint32_t> swap_chain::acquire_next_image() {
	vkWaitForFences(m_device.handle(), 1, &m_in_flight_fences[m_current_frame],
		VK_TRUE, constants::fence_wait_timeout);

	uint32_t image_index;
	const auto result{ vkAcquireNextImageKHR(
		m_device.handle(),
		m_swap_chain,
		constants::acquire_next_timeout,
		m_available_images_semaphores[m_current_frame],
		VK_NULL_HANDLE,
		&image_index
	) };
	return VK_SUCCESS == result ? std::make_optional(image_index) : std::nullopt;
}

VkResult swap_chain::submit(uint32_t image_index, const VkCommandBuffer *buffers, uint32_t buffers_count) {
	if (auto current_image_fence{ m_images_in_flight[image_index] }; current_image_fence != VK_NULL_HANDLE) {
		vkWaitForFences(m_device.handle(), 1, &current_image_fence, VK_TRUE, constants::fence_wait_timeout);
	}
	const auto *current_fence_ptr{ std::next(std::data(m_in_flight_fences), m_current_frame) };
	m_images_in_flight[image_index] = *current_fence_ptr;

	const std::array<VkPipelineStageFlags, 1> wait_stages{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	const auto *signal_semaphores_ptr{
		std::next(std::data(m_render_finished_semaphores), m_current_frame)
	};
	const VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount   = 1,
		.pWaitSemaphores      = std::next(std::data(m_available_images_semaphores), m_current_frame),
		.pWaitDstStageMask    = std::data(wait_stages),
		.commandBufferCount   = buffers_count,
		.pCommandBuffers      = buffers,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores    = signal_semaphores_ptr,
	};

	vkResetFences(m_device.handle(), 1, current_fence_ptr);
	if (VK_SUCCESS != vkQueueSubmit(m_device.graphics_queue(), 1, &submit_info, *current_fence_ptr)) {
		throw swap_chain_error{ "Failed to submit draw command buffer" };
	}

	const VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = signal_semaphores_ptr,
		.swapchainCount     = 1,
		.pSwapchains        = &m_swap_chain,
		.pImageIndices      = &image_index,
		.pResults           = nullptr
	};
	const auto result{ vkQueuePresentKHR(m_device.present_queue(), &present_info) };
	m_current_frame = (m_current_frame + 1) % constants::max_frames_in_flight;
	return result;
}

#pragma region construct methods

void swap_chain::construct_swap_chain() {
	const auto support{ m_device.query_swap_chain_support() };
	const uint32_t image_count{ [] (const auto &capabilities) {
		const uint32_t count{ capabilities.minImageCount + 1 };
		if (capabilities.maxImageCount > 0 && count > capabilities.maxImageCount) {
			return capabilities.maxImageCount;
		}
		return count;
	}(support.capabilities) };

	auto [graphics_family, present_family]{ m_device.find_queue_families() };
	// TODO: Check optional families. It actually matters
	constexpr uint32_t queue_family_indices_count{ 2 };
	const std::array<uint32_t, queue_family_indices_count> queue_family_indices{
		graphics_family.value_or(0),
		present_family.value_or(0)
	};
	const bool is_concurrent{ graphics_family == present_family };
	const auto sharing_mode{ is_concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE };

	const auto surface_format{ select_surface_format(support.formats) };
	const VkSwapchainCreateInfoKHR create_info{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface               = m_device.surface(),
		.minImageCount         = image_count,
		.imageFormat           = surface_format.format,
		.imageColorSpace       = surface_format.colorSpace,
		.imageExtent           = select_extent(support.capabilities),
		.imageArrayLayers      = 1,
		.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode      = sharing_mode,
		.queueFamilyIndexCount = (is_concurrent ? queue_family_indices_count      : 0u     ),
		.pQueueFamilyIndices   = (is_concurrent ? std::data(queue_family_indices) : nullptr),
		.preTransform          = support.capabilities.currentTransform,
		.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode           = select_present_mode(support.present_modes),
		.clipped               = VK_TRUE,
		.oldSwapchain          = VK_NULL_HANDLE
	};
	if (VK_SUCCESS != vkCreateSwapchainKHR(m_device.handle(), &create_info, nullptr, &m_swap_chain)) {
		throw swap_chain_error{ "Failed to create swap chain." };
	}

	uint32_t total_images_count{};
	vkGetSwapchainImagesKHR(m_device.handle(), m_swap_chain, &total_images_count, nullptr);
	m_images.resize(total_images_count);
	vkGetSwapchainImagesKHR(m_device.handle(), m_swap_chain, &total_images_count, std::data(m_images));

	m_image_format = create_info.imageFormat;
	m_extent = create_info.imageExtent;
}

void swap_chain::construct_image_views() {
	m_image_views.resize(std::size(m_images));
	const auto device{ m_device.handle() };
	for (size_t i{}; i < std::size(m_image_views); ++i) {
		const VkImageViewCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = m_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_image_format,
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1
			}
		};
		if (vkCreateImageView(device, &create_info, nullptr, &m_image_views[i]) != VK_SUCCESS) {
			throw swap_chain_error{ "Failed to create texture image view." };
		}
	}
}

void swap_chain::construct_render_pass() {
	enum { COLOR_ATTACHMENT, DEPTH_ATTACHMENT, ATTACHMENTS_COUNT };

	const std::array<const VkAttachmentDescription, ATTACHMENTS_COUNT> attachments{
		VkAttachmentDescription{
			.format         = m_image_format,
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		VkAttachmentDescription{
			.format         = find_depth_format(),
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	const VkAttachmentReference color_attachment_ref{
		.attachment = COLOR_ATTACHMENT,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference depth_attachment_ref{
		.attachment = DEPTH_ATTACHMENT,
		.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkSubpassDescription sub_pass{
		.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount    = 1,
		.pColorAttachments       = &color_attachment_ref,
		.pDepthStencilAttachment = &depth_attachment_ref
	};
	const VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.srcStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask
			= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
			| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

	const VkRenderPassCreateInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(std::size(attachments)),
		.pAttachments    = std::data(attachments),
		.subpassCount    = 1,
		.pSubpasses      = &sub_pass,
		.dependencyCount = 1,
		.pDependencies   = &dependency
	};
	if (VK_SUCCESS != vkCreateRenderPass(m_device.handle(), &render_pass_info, nullptr, &m_render_pass)) {
		throw swap_chain_error{ "Failed to create render pass." };
	}
}

void swap_chain::construct_depth_resources() {
	m_depth_images.resize(std::size(m_images));
	m_depth_image_memories.resize(std::size(m_images));
	m_depth_image_views.resize(std::size(m_images));

	const auto device{ m_device.handle() };
	const auto depth_format{ find_depth_format() };
	for (size_t i{}; i < std::size(m_depth_images); ++i) {
		m_depth_images[i] = m_device.make_image(
			VkImageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType   = VK_IMAGE_TYPE_2D,
				.format      = depth_format,
				.extent      = VkExtent3D{
					.width  = m_extent.width,
					.height = m_extent.height,
					.depth  = 1
				},
				.mipLevels   = 1,
				.arrayLayers = 1,
				.samples     = VK_SAMPLE_COUNT_1_BIT,
				.tiling      = VK_IMAGE_TILING_OPTIMAL,
				.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			},
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_depth_image_memories[i]
		);
		const VkImageViewCreateInfo view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image            = m_depth_images[i],
			.viewType         = VK_IMAGE_VIEW_TYPE_2D,
			.format           = depth_format,
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel   = 0, .levelCount = 1,
				.baseArrayLayer = 0, .layerCount = 1,
			}
		};
		if (VK_SUCCESS != vkCreateImageView(device, &view_info, nullptr, &m_depth_image_views[i])) {
			throw swap_chain_error{ "Failed to create texture image view." };
		}
	}
}

void swap_chain::construct_framebuffers() {
	m_framebuffers.resize(std::size(m_images));
	const auto device{ m_device.handle() };
	for (size_t i{}; i < std::size(m_framebuffers); ++i) {
		const std::array attachments{ m_image_views[i], m_depth_image_views[i] };
		const VkFramebufferCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass      = m_render_pass,
			.attachmentCount = static_cast<uint32_t>(std::size(attachments)),
			.pAttachments    = std::data(attachments),
			.width           = m_extent.width,
			.height          = m_extent.height,
			.layers          = 1
		};
		if (VK_SUCCESS != vkCreateFramebuffer(device, &create_info, nullptr, &m_framebuffers[i])) {
			throw swap_chain_error{ "Failed to create a framebuffer." };
		}
	}
}

void swap_chain::construct_sync_objects() {
	m_images_in_flight.resize(std::size(m_images), VK_NULL_HANDLE);

	const VkSemaphoreCreateInfo semaphore_info{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	const VkFenceCreateInfo fence_info{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	const auto device{ m_device.handle() };
	for (size_t i{}; i < std::size(m_available_images_semaphores); ++i) {
		const auto result{
			vkCreateSemaphore(device, &semaphore_info, nullptr, &m_available_images_semaphores[i]) &
			vkCreateSemaphore(device, &semaphore_info, nullptr, &m_render_finished_semaphores[i])  &
			vkCreateFence(device, &fence_info, nullptr, &m_in_flight_fences[i])
		};
		if (result != VK_SUCCESS) {
			throw swap_chain_error{ "Failed to create syncronization objects for a frame." };
		}
	}
}

#pragma endregion construct methods

#pragma region select methods

VkSurfaceFormatKHR swap_chain::select_surface_format(const std::vector<VkSurfaceFormatKHR> &available) {
	static constexpr auto required_format_and_color_space{ [] (const auto &surface) {
		return (surface.format     == VK_FORMAT_B8G8R8A8_SRGB)
		&&     (surface.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
	} };
	namespace stdr = std::ranges;
	if (auto found{ stdr::find_if(available, required_format_and_color_space) }; found != std::end(available)) {
		return *found;
	}
	return available.front();
}

VkPresentModeKHR swap_chain::select_present_mode(const std::vector<VkPresentModeKHR> &available) {
	namespace stdr = std::ranges;
	if (auto found{ stdr::find(available, VK_PRESENT_MODE_MAILBOX_KHR) }; found != std::end(available)) {
		return *found;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D swap_chain::select_extent(const VkSurfaceCapabilitiesKHR &caps) {
	if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return caps.currentExtent;
	}

	return VkExtent2D{
		.width  = std::clamp(m_extent.width, caps.minImageExtent.width, caps.maxImageExtent.width),
		.height = std::clamp(m_extent.height, caps.minImageExtent.height, caps.maxImageExtent.height),
	};
}

#pragma endregion select methods

} // namespace vc::engine::graphics
