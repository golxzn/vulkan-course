#include <cassert>

#include "engine/resources/model.hpp"

namespace vc::engine::resources {

model::model(graphics::device &device, const std::span<const vertex> vertices) : m_device{ device } {
	construct_vertex_buffers(vertices);
}

model::~model() {
	const auto device{ m_device.handle() };
	vkDestroyBuffer(device, m_vertex_buffer, nullptr);
	vkFreeMemory(device, m_vertex_buffer_memory, nullptr);
}


void model::bind(VkCommandBuffer command_buffer) {
	const std::array buffers{ m_vertex_buffer };
	const std::array offsets{ VkDeviceSize{ 0 } };

	vkCmdBindVertexBuffers(command_buffer, 0,
		static_cast<u32>(std::size(buffers)),
		std::data(buffers), std::data(offsets)
	);
}

void model::draw(VkCommandBuffer command_buffer) {
	vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
}

void model::construct_vertex_buffers(const std::span<const vertex> vertices) {
	constexpr auto vertex_size{ static_cast<u32>(sizeof(decltype(vertices)::value_type)) };

	m_vertex_count = static_cast<u32>(std::size(vertices));
	assert(m_vertex_count >= 3 && "Vertex count should be at least 3");

	const VkDeviceSize buffer_size{ vertex_size * m_vertex_count };

	m_vertex_buffer = m_device.make_buffer(
		buffer_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_vertex_buffer_memory
	);

	void *data{ nullptr };
	vkMapMemory(m_device.handle(), m_vertex_buffer_memory, 0, buffer_size, 0, &data);
	std::memcpy(data, std::data(vertices), static_cast<size_t>(buffer_size));
	vkUnmapMemory(m_device.handle(), m_vertex_buffer_memory);
}

#pragma region vertex


auto model::vertex::binding_description()
	-> std::array<VkVertexInputBindingDescription, constants::bindings_count> {
	return {
		VkVertexInputBindingDescription{
			.binding   = 0,
			.stride    = sizeof(vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		}
	};
}

auto model::vertex::attribute_description()
	-> std::array<VkVertexInputAttributeDescription, constants::vertex_elements> {
	return {
		VkVertexInputAttributeDescription{
			.location = 0,
			.binding  = 0,
			.format   = VK_FORMAT_R32G32B32_SFLOAT,
			.offset   = 0
		},
		VkVertexInputAttributeDescription{
			.location = 1,
			.binding  = 0,
			.format   = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset   = sizeof(glm::vec3)
		}
	};
}


#pragma endregion vertex

} // namespace vc::engine::resources

