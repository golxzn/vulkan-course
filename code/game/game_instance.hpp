#pragma once

#include "core/window.hpp"
#include "engine/graphics/device.hpp"
#include "engine/graphics/pipeline.hpp"

namespace vc::game {

namespace constants {

constexpr glm::i32vec2     window_size { 1024, 720       };
constexpr std::string_view window_title{ "Vulkan Course" };
constexpr std::string_view default_shader{ "assets/shaders/primitive/primitive" };

} // namespace constants


class game_instance {
public:
	int run();

private:
	core::window               m_window  { constants::window_size, constants::window_title };
	engine::graphics::device   m_device  { m_window };
	engine::graphics::pipeline m_pipeline{ m_device, constants::default_shader,
		engine::graphics::pipeline_config{ constants::window_size } };
};

} // namespace vc::game
