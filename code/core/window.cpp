
#include "core/window.hpp"
#include "engine/graphics/vulkan-instance.hpp"

namespace vc::core {

#if defined(VC_DEBUG)
namespace {

void glfw_error_callback(int error_code, const char* description) {
	std::printf("[GLFW  ][ERR ] Error #%d: %s\n", error_code, description);
}

} // anonymous namespace
#endif // defined(VC_DEBUG)

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

VkSurfaceKHR window::make_surface(const engine::graphics::vulkan_instance &instance) noexcept {
	if (VkSurfaceKHR surface;
		glfwCreateWindowSurface(instance.handle(), m_window.get(), nullptr, &surface) == VK_SUCCESS) {
		return surface;
	}

	return VK_NULL_HANDLE;
}

VkExtent2D window::extent() const noexcept {
	int32_t width;
	int32_t height;
	glfwGetWindowSize(m_window.get(), &width, &height);
	return VkExtent2D{
		.width  = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
	};
}

glfw::unique_window window::make_instance(const glm::i32vec2 size, const std::string_view title) {
	static bool instantiated{ false };
	if (instantiated) {
		std::fprintf(stderr, "[window] Fatal error! The second instance is not allowed.\n");
		std::terminate();
	}

	instantiated = true;

	glfwInit();
#if defined(VC_DEBUG)
	glfwSetErrorCallback(glfw_error_callback);
#endif // defined(VC_DEBUG)

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
