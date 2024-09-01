#include "engine/graphics/pipeline-config.hpp"

namespace vc::engine::graphics {

pipeline_config::pipeline_config(const glm::vec2 size) noexcept
	: viewport{ .width = size.x, .height = size.y }
	, scissor { .extent = { static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y) } } {}

} // namespace vc::engine::graphics
