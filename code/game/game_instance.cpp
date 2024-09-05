#include <fmt/core.h>

#include "game/game_instance.hpp"

namespace vc::game {

game_instance::game_instance() {
	construct_pipeline();
	construct_command_buffers();
}

int game_instance::run() {
	while (!m_window.is_closing()) {
		m_window.pull_events();
		render_frame();
	}
	m_device.wait_for_idle();

	return EXIT_SUCCESS;
}

void game_instance::render_frame() {
	const auto image_index{ m_swap_chain.acquire_next_image() };
	if (!image_index.has_value()) {
		throw game_instance_error{ "Failed to acquire next image." };
	}

	if (VK_SUCCESS != m_swap_chain.submit(*image_index, &m_command_buffers[*image_index])) {
		throw game_instance_error{ fmt::format("Failed to submit frame buffer #{}", *image_index) };
	}
}

void game_instance::construct_pipeline() {
	const auto extent{ m_swap_chain.extent() };
	m_pipeline.emplace(m_device, constants::default_shader, engine::graphics::pipeline_config{
		.viewport = {
			.width  = static_cast<f32>(extent.width),
			.height = static_cast<f32>(extent.height),
		},
		.scissor     = { .extent = extent },
		.layout      = static_cast<VkPipelineLayout>(m_pipeline_layout),
		.render_pass = m_swap_chain.render_pass()
	});
}

void game_instance::construct_command_buffers() {
	m_command_buffers.resize(m_swap_chain.image_count());

	const VkCommandBufferAllocateInfo allocate_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = m_device.command_pool(),
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<u32>(std::size(m_command_buffers))
	};

	if (VK_SUCCESS != vkAllocateCommandBuffers(m_device.handle(), &allocate_info, std::data(m_command_buffers))) {
		throw game_instance_error{ fmt::format(
			"Cannot allocate {} command buffers.", std::size(m_command_buffers)
		) };
	}

	for (size_t i{}; i < std::size(m_command_buffers); ++i) {
		auto &command_buffer{ m_command_buffers[i] };
		const VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};
		if (VK_SUCCESS != vkBeginCommandBuffer(command_buffer, &begin_info)) {
			throw game_instance_error{ fmt::format("Failed to begin command buffer #{}.", i) };
		}

		const VkRenderPassBeginInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_swap_chain.render_pass(),
			.framebuffer = m_swap_chain.framebuffer(i),
			.renderArea = VkRect2D{
				.offset = { 0, 0 },
				.extent = m_swap_chain.extent()
			},
			.clearValueCount = static_cast<u32>(std::size(constants::clear_values)),
			.pClearValues    = std::data(constants::clear_values)
		};
		vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		m_pipeline->bind(command_buffer);
		vkCmdDraw(command_buffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(command_buffer);
		if (VK_SUCCESS != vkEndCommandBuffer(command_buffer)) {
			throw game_instance_error{ fmt::format("Failed to end command buffer #{}.", i) };
		}
	}
}

} // namespace vc::game
