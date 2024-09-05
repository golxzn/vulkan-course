#pragma once

#include <span>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "engine/graphics/device.hpp"

namespace vc::engine::resources {

namespace constants {

constexpr size_t bindings_count { 1 };
constexpr size_t vertex_elements{ 2 };

} // namespace constants


class model {
public:
	struct vertex;

	model(graphics::device &device, const std::span<const vertex> vertices);
	~model();

	model(const model &) = delete;
	model &operator=(const model &) = delete;

	void bind(VkCommandBuffer command_buffer);
	void draw(VkCommandBuffer command_buffer);

private:
	graphics::device &m_device;
	VkBuffer          m_vertex_buffer;
	VkDeviceMemory    m_vertex_buffer_memory;
	u32               m_vertex_count;

	void construct_vertex_buffers(const std::span<const vertex> vertices);
};

struct model::vertex {
	glm::vec3 position;
	glm::vec4 color;


	[[nodiscard]] static auto binding_description()
		-> std::array<VkVertexInputBindingDescription, constants::bindings_count>;

	[[nodiscard]] static auto attribute_description()
		-> std::array<VkVertexInputAttributeDescription, constants::vertex_elements>;
};

} // namespace vc::engine::resources
