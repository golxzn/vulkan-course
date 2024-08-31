#pragma once

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace glfw {

struct window_destroy_functor final {
	void operator()(GLFWwindow *window) {
		glfwDestroyWindow(window);
	}
};

using unique_window = std::unique_ptr<GLFWwindow, window_destroy_functor>;

} // namespace glfw
