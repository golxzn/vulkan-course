
#include "core/window.hpp"

namespace vc::core {

window::window(const glm::i32vec2 size, const std::string_view title)
	: m_window{ make_instance(size, title) } {}

window::~window() {
	glfwTerminate();
}

void window::pull_events() {
	glfwPollEvents();
}

bool window::is_closing() const noexcept {
	return glfwWindowShouldClose(m_window.get());
}

glfw::unique_window window::make_instance(const glm::i32vec2 size, const std::string_view title) {
	static bool instantiated{ false };
	if (instantiated) {
		std::fprintf(stderr, "[window] Fatal error! The second instance is not allowed.\n");
		std::terminate();
	}

	instantiated = true;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

	return glfw::unique_window{ glfwCreateWindow(
		size.x, size.y,
		std::data(title),
		nullptr,
		nullptr
	) };
}

} // namespace vc::core
