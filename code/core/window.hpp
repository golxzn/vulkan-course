#pragma once

#include <optional>
#include <string_view>
#include <glm/vec2.hpp>

#include "core/glfw-header.hpp"

namespace vc::engine::graphics { class vulkan_instance; }

namespace vc::core {

class window {
public:
	explicit window(glm::i32vec2 size, std::string_view title);
	~window();

	window(const window &other) = delete;
	window(window &&other) noexcept = default;
	window &operator=(const window &other) = delete;
	window &operator=(window &&other) noexcept = default;

	void pull_events();

	[[nodiscard]] bool is_closing() const noexcept;
	[[nodiscard]] auto make_surface(const engine::graphics::vulkan_instance &instance) noexcept -> VkSurfaceKHR;
	[[nodiscard]] auto extent() const noexcept -> VkExtent2D;

private:
	glfw::unique_window m_window;

	static glfw::unique_window make_instance(glm::i32vec2 size, std::string_view title);

};

} // namespace vc::core
