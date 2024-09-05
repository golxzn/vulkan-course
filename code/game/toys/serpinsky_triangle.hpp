#pragma once

#include <glm/common.hpp>

#include "engine/resources/model.hpp"

namespace vc::game::toys {

using vertex_type = engine::resources::model::vertex;

[[nodiscard]] std::vector<vertex_type> populate(const std::span<const vertex_type> vertices);
[[nodiscard]] std::vector<vertex_type> make_serpinsky(size_t depth,
	const std::span<const vertex_type> vertices);

} // namespace vc::game::toys
