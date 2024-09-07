#pragma once

#include <glm/mat4x4.hpp>

#include "core/window.hpp"
#include "engine/graphics/device.hpp"
#include "engine/graphics/pipeline.hpp"
#include "engine/graphics/swap-chain.hpp"
#include "engine/graphics/vulkan-instance.hpp"

#include "engine/resources/model.hpp"

namespace vc::game {

namespace constants {

constexpr glm::i32vec2     window_size   { 1024, 720       };
constexpr std::string_view default_shader{ "assets/shaders/primitive/primitive" };
constexpr std::array<VkClearValue, 2> clear_values{
	VkClearValue{ .color = { 0.12f, 0.12f, 0.16f, 1.0f } },
	VkClearValue{ .depthStencil = { 1.0f, 0 } }
};

} // namespace constants


class game_instance {
public:
	game_instance();

	game_instance(const game_instance &) = delete;
	game_instance &operator=(const game_instance &) = delete;

	int run();

private:
	core::window                              m_window         { constants::window_size };
	engine::graphics::vulkan_instance         m_instance       {};
	engine::graphics::device                  m_device         { m_instance, m_window };
	engine::graphics::swap_chain              m_swap_chain     { m_device, m_window.extent() };
	std::optional<engine::graphics::pipeline_layout> m_pipeline_layout;
	std::optional<engine::graphics::pipeline> m_pipeline;
	std::vector<VkCommandBuffer>              m_command_buffers;
	std::unique_ptr<engine::resources::model> m_model;

	glm::mat4 test_model_transform{ 1.0f };

	void update(double delta);
	void render_frame();

	void construct_pipeline();
	void construct_command_buffers();

	void record_command_buffer(size_t image_index);

	void load_models();
};

class game_instance_error : public std::runtime_error {
public:
	using base_type = std::runtime_error;
	using base_type::runtime_error;
};

} // namespace vc::game
