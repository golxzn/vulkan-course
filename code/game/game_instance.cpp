#include <fmt/core.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "game/game_instance.hpp"
#include "game/toys/serpinsky_triangle.hpp"

namespace vc::game {

struct simple_push_constant_data {
	glm::mat4 transform;
};

game_instance::game_instance() {
	load_models();
	construct_pipeline();
	construct_command_buffers();
}

int game_instance::run() {
	constexpr float move_step{ 0.01f };

	double last_time{ glfwGetTime() };
	while (!m_window.is_closing()) {
		const double delta{ glfwGetTime() - last_time };
		last_time = glfwGetTime();

		m_window.pull_events();

		update(delta);
		render_frame();
	}
	m_device.wait_for_idle();

	return EXIT_SUCCESS;
}

void game_instance::update(const double delta) {
	constexpr glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };
	test_model_transform = glm::rotate(test_model_transform, static_cast<float>(delta), rotation_axis);
}

void game_instance::render_frame() {
	const auto image_index{ m_swap_chain.acquire_next_image() };
	if (!image_index.has_value()) {
		throw game_instance_error{ "Failed to acquire next image." };
	}

	record_command_buffer(*image_index);

	if (VK_SUCCESS != m_swap_chain.submit(*image_index, &m_command_buffers[*image_index])) {
		throw game_instance_error{ fmt::format("Failed to submit frame buffer #{}", *image_index) };
	}
}

void game_instance::construct_pipeline() {
	const std::array<VkPushConstantRange, 1> ranges{
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset     = 0,
			.size       = sizeof(simple_push_constant_data)
		}
	};

	m_pipeline_layout.emplace(m_device, std::span<const VkDescriptorSetLayout>{}, ranges);

	const auto extent{ m_swap_chain.extent() };
	m_pipeline.emplace(m_device, constants::default_shader, engine::graphics::pipeline_config{
		.viewport = {
			.width  = static_cast<f32>(extent.width),
			.height = static_cast<f32>(extent.height),
		},
		.scissor     = { .extent = extent },
		.layout      = static_cast<VkPipelineLayout>(*m_pipeline_layout),
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
		record_command_buffer(i);
	}
}

void game_instance::record_command_buffer(size_t image_index) {
	auto &command_buffer{ m_command_buffers[image_index] };
	const VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	if (VK_SUCCESS != vkBeginCommandBuffer(command_buffer, &begin_info)) {
		throw game_instance_error{ fmt::format("Failed to begin command buffer #{}.", image_index) };
	}

	const VkRenderPassBeginInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_swap_chain.render_pass(),
		.framebuffer = m_swap_chain.framebuffer(image_index),
		.renderArea = VkRect2D{
			.offset = { 0, 0 },
			.extent = m_swap_chain.extent()
		},
		.clearValueCount = static_cast<u32>(std::size(constants::clear_values)),
		.pClearValues    = std::data(constants::clear_values)
	};
	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	m_pipeline->bind(command_buffer);

	m_model->bind(command_buffer);

	const simple_push_constant_data constant_data{
		.transform = test_model_transform
	};
	vkCmdPushConstants(command_buffer, static_cast<VkPipelineLayout>(*m_pipeline_layout),
		VK_SHADER_STAGE_VERTEX_BIT,
		0, sizeof(simple_push_constant_data), &constant_data);

	m_model->draw(command_buffer);

	vkCmdEndRenderPass(command_buffer);
	if (VK_SUCCESS != vkEndCommandBuffer(command_buffer)) {
		throw game_instance_error{ fmt::format("Failed to end command buffer #{}.", image_index) };
	}
}

void game_instance::load_models() {
	using namespace engine;
	using vertex = resources::model::vertex;

	constexpr std::array vertices{
		vertex{ .position = {  0.0f, -0.9f, 0.0f }, .color = { 1.0f, 0.62f, 0.23f, 1.0f } },
		vertex{ .position = {  0.9f,  0.9f, 0.0f }, .color = { 0.5f, 0.31f, 0.61f, 1.0f } },
		vertex{ .position = { -0.9f,  0.9f, 0.0f }, .color = { 0.5f, 0.31f, 0.61f, 1.0f } }
	};

	m_model = std::make_unique<resources::model>(m_device, toys::make_serpinsky(6, vertices));
}

} // namespace vc::game
