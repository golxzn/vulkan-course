#include "game/toys/serpinsky_triangle.hpp"

namespace vc::game::toys {

using vertex_type = engine::resources::model::vertex;

std::vector<vertex_type> populate(const std::span<const vertex_type> vertices) {
	if (std::size(vertices) < 3) return std::vector<vertex_type>{};

	const auto &top  { vertices[0] };
	const auto &right{ vertices[1] };
	const auto &left { vertices[2] };

	const vertex_type top_right{
		.position = (top.position + right.position) * glm::vec3{ 0.5 },
		.color = glm::mix(top.color, right.color, 0.5f)
	};
	const vertex_type right_left{
		.position = (right.position + left.position) * glm::vec3{ 0.5 },
		.color = glm::mix(right.color, left.color, 0.5f)
	};
	const vertex_type left_top{
		.position = (left.position + top.position) * glm::vec3{ 0.5 },
		.color = glm::mix(left.color, top.color, 0.5f)
	};

	return std::vector<vertex_type> {
		top, top_right, left_top,

		top_right, right, right_left,

		left_top, right_left, left
	};
}

std::vector<vertex_type> make_serpinsky(const size_t depth, const std::span<const vertex_type> vertices) {
	if (depth == 0) return std::vector<vertex_type>{ std::begin(vertices), std::end(vertices) };

	constexpr size_t vertices_count{ 3 };

	std::vector<vertex_type> result(std::size(vertices) * vertices_count);
	auto insert_pos{ std::begin(result) };
	for (size_t i{}; i < std::size(vertices); i += vertices_count) {
		const auto populated{ populate(std::span<const vertex_type>{ &vertices[i], vertices_count }) };
		std::ranges::copy(populated, insert_pos);
		insert_pos += std::size(populated);
	}
	return make_serpinsky(depth - 1, result);
}


} // namespace vc::game::toys
