#include "engine/graphics/pipeline-config.hpp"

namespace vc::engine::graphics {

pipeline_config::pipeline_config(const glm::vec2 size) noexcept
	: viewport{ .width = size.x, .height = size.y }
	, scissor { .extent = { static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y) } } {}

pipeline_config::pipeline_config(const VkExtent2D size) noexcept
	: viewport{ .width = static_cast<float>(size.width), .height = static_cast<float>(size.height) }
	, scissor { .extent = size } {}



} // namespace vc::engine::graphics
