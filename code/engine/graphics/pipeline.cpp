#include <cassert>
#include <fstream>
#include <algorithm>

#include <fmt/core.h>

#include "engine/graphics/device.hpp"
#include "engine/graphics/pipeline.hpp"

namespace vc::engine::graphics {

pipeline::pipeline(device &dev, const std::string_view shader, const pipeline_config &config)
	: m_device{ dev } {

	assert(config.layout != VK_NULL_HANDLE
		&& "at pipeline constructor: The config should provide a pipeline `layout`, but it's null");
	assert(config.render_pass != VK_NULL_HANDLE
		&& "at pipeline constructor: The config should provide a `render_pass`, but it's null");

	const auto shaders_count{ load_shaders(shader) };
	if (shaders_count == 0) {
		throw pipeline_error{ fmt::format(R"(Cannot find any shader file of "{}")", shader) };
	}

	static constexpr auto to_vk{ [](const auto type) {
		switch (type) {
			using enum shader_type;
			case vertex:                  return VK_SHADER_STAGE_VERTEX_BIT;
			case fragment:                return VK_SHADER_STAGE_FRAGMENT_BIT;
			case geometry:                return VK_SHADER_STAGE_GEOMETRY_BIT;
			case tessellation_control:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case tessellation_evaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case compute:                 return VK_SHADER_STAGE_COMPUTE_BIT;
			default: break;
		}
		return VkShaderStageFlagBits{};
	} };

	std::vector<VkPipelineShaderStageCreateInfo> stages;
	stages.reserve(shaders_count);

	for (const auto &[type, shader_module] : m_shaders) {
		if (shader_module == nullptr) continue;

		stages.emplace_back(
			/*.sType  = */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			/*.pNext  = */ nullptr,
			/*.flags  = */ 0,
			/*.stage  = */ to_vk(type),
			/*.module = */ shader_module,
			/*.pName  = */ std::data(constants::shader_stage_entry_point),
			/*.pSpecializationInfo = */ nullptr
		);
	}

	const VkPipelineVertexInputStateCreateInfo vertex_input_create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount   = 0,
		.pVertexBindingDescriptions      = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions    = nullptr
	};

	const VkGraphicsPipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount          = static_cast<uint32_t>(std::size(stages)),
		.pStages             = std::data(stages),
		.pVertexInputState   = &vertex_input_create_info,
		.pInputAssemblyState = &config.input_assembly_create_info,
		.pTessellationState  = nullptr,
		.pViewportState      = &config.viewport_info,
		.pRasterizationState = &config.rasterization_create_info,
		.pMultisampleState   = &config.multi_sample_create_info,
		.pDepthStencilState  = &config.depth_stencil_create_info,
		.pColorBlendState    = &config.color_blend_create_info,
		.pDynamicState       = nullptr,
		.layout              = config.layout,
		.renderPass          = config.render_pass,
		.subpass             = config.sub_pass,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1
	};

	const auto status{ vkCreateGraphicsPipelines(
		dev.device_handle(),
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		nullptr,
		&m_pipeline
	) };

	if (VK_SUCCESS != status) {
		throw pipeline_error{ "Cannot create graphics pipeline." };
	}
}

pipeline::~pipeline() {
	const auto device{ m_device.device_handle() };
	for (const auto &[_, shader_module] : m_shaders) {
		if (shader_module != nullptr) {
			vkDestroyShaderModule(device, shader_module, nullptr);
		}
	}

	vkDestroyPipeline(device, m_pipeline, nullptr);
}

size_t pipeline::load_shaders(const std::string_view shader) {
	const auto name_length{ std::size(shader) };

	std::string filename(
		name_length + constants::shader_file_extention_size +
		std::size(constants::compiled_shader_file_extension), '\0'
	);
	std::copy_n(std::begin(shader), name_length, std::begin(filename));
	std::copy_n(std::rbegin(constants::compiled_shader_file_extension),
		std::size(constants::compiled_shader_file_extension), std::rbegin(filename));

	size_t loaded_counter{};
	for (const auto &[type, extension] : constants::shader_extensions) {
		std::copy_n(std::begin(extension), std::size(extension),
			std::next(std::begin(filename), name_length));

		if (const auto content{ load_file(filename) }; !std::empty(content)) {
			m_shaders.insert_or_assign(type, make_shader(filename, content));
			++loaded_counter;
		}
	}
	return loaded_counter;
}

VkShaderModule pipeline::make_shader(const std::string_view filename, const std::vector<char> &code) {
	const VkShaderModuleCreateInfo create_info{
		.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = static_cast<uint32_t>(std::size(code)),
		.pCode    = reinterpret_cast<const uint32_t *>(std::data(code))
	};

	VkShaderModule shader;
	if (VK_SUCCESS != vkCreateShaderModule(m_device.device_handle(), &create_info, nullptr, &shader)) {
		throw pipeline_error{ fmt::format(
			R"(Failed to create shader module from "%s" file.)", filename
		) };
	}
	return shader;
}

std::vector<char> pipeline::load_file(const std::string_view filename) {
	if (std::ifstream file{ std::data(filename), std::ios::ate | std::ios::binary }; file.is_open()) {
		std::vector<char> content( static_cast<size_t>(file.tellg()) );
		file.seekg(std::ios::beg);
		file.read(std::data(content), std::size(content));
		return std::move(content);
	}

	return {};
}

} // namespace vc::engine::graphics
