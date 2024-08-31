#pragma once

#include "core/window.hpp"

namespace vc::game {

namespace constants {

constexpr glm::i32vec2     window_size { 1024, 720       };
constexpr std::string_view window_title{ "Vulkan Course" };

} // namespace constants


class game_instance {
public:
	int run();

private:
	core::window m_window{ constants::window_size, constants::window_title };
};

} // namespace vc::game
