#include <cstdio>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto window{ glfwCreateWindow(1024, 720, "Vulkan Course", nullptr, nullptr) };

	uint32_t extensions_count{};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);

	std::printf("Vulkan extensions count: %u", extensions_count);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(std::exchange(window, nullptr));
	glfwTerminate();
}