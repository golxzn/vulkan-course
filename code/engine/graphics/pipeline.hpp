#pragma once

#include <vector>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "engine/graphics/pipeline-config.hpp"

namespace vc::engine::graphics {

class device;

class pipeline_error : public std::runtime_error {
public:
	using base_type = std::runtime_error;
	using base_type::runtime_error;
};

enum class shader_type : uint8_t {
	unknown,
	vertex,
	fragment,
	geometry,
	tessellation_control,
	tessellation_evaluation,
	compute,
};

namespace constants {

constexpr std::string_view shader_stage_entry_point      { "main"          };
constexpr std::string_view compiled_shader_file_extension{ ".spv"          };
constexpr size_t           shader_file_extention_size    { sizeof(".vert") };
constexpr size_t           content_buffer_initial_size   { 2048            };

static const std::unordered_map<shader_type, std::string_view> shader_extensions{
	{ shader_type::vertex,                   ".vert" },
	{ shader_type::fragment,                 ".frag" },
	{ shader_type::geometry,                 ".geom" },
	{ shader_type::tessellation_control,     ".tesc" },
	{ shader_type::tessellation_evaluation,  ".tese" },
	{ shader_type::compute,                  ".comp" },
};

} // namespace constants

class pipeline {
public:
	explicit pipeline(device &device, std::string_view shader, const pipeline_config &config);
	~pipeline();

	pipeline(const pipeline &) = delete;
	pipeline &operator=(const pipeline &) = delete;

	void bind(VkCommandBuffer buffer, VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
	device &m_device;
	std::unordered_map<shader_type, VkShaderModule> m_shaders{
		{ shader_type::vertex,                   nullptr },
		{ shader_type::fragment,                 nullptr },
		{ shader_type::geometry,                 nullptr },
		{ shader_type::tessellation_control,     nullptr },
		{ shader_type::tessellation_evaluation,  nullptr },
		{ shader_type::compute,                  nullptr },
	};
	VkPipeline m_pipeline{ VK_NULL_HANDLE };

	void construct_pipeline();
	auto load_shaders(const std::string_view shader) -> size_t;
	auto make_shader(const std::string_view filename, const std::vector<char> &code)
		-> VkShaderModule;

	static bool load_file_to(std::vector<char> &buffer, std::string_view filename);

};

} // namespace vc::engine::graphics
