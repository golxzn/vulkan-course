#include <cassert>
#include <fstream>
#include <algorithm>

#include <fmt/core.h>

#include "engine/graphics/device.hpp"
#include "engine/graphics/pipeline.hpp"

namespace vc::engine::graphics {

#pragma region pipeline

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
	const VkPipelineViewportStateCreateInfo viewport_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports    = &config.viewport,
		.scissorCount  = 1,
		.pScissors     = &config.scissor
	};
	const VkGraphicsPipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount          = static_cast<u32>(std::size(stages)),
		.pStages             = std::data(stages),
		.pVertexInputState   = &vertex_input_create_info,
		.pInputAssemblyState = &config.input_assembly_create_info,
		.pTessellationState  = nullptr,
		.pViewportState      = &viewport_state,
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
		dev.handle(),
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
	const auto device{ m_device.handle() };
	for (const auto &[_, shader_module] : m_shaders) {
		if (shader_module != nullptr) {
			vkDestroyShaderModule(device, shader_module, nullptr);
		}
	}

	vkDestroyPipeline(device, m_pipeline, nullptr);
}

void pipeline::bind(VkCommandBuffer buffer, const VkPipelineBindPoint bind_point) {
	vkCmdBindPipeline(buffer, bind_point, m_pipeline);
}

size_t pipeline::load_shaders(const std::string_view shader) {
	const auto name_length{ std::size(shader) };

	std::string filename(
		name_length + constants::shader_file_extention_size +
		std::size(constants::compiled_shader_file_extension) - 1, '\0'
	);
	std::copy_n(std::begin(shader), name_length, std::begin(filename));
	std::copy_n(std::rbegin(constants::compiled_shader_file_extension),
		std::size(constants::compiled_shader_file_extension), std::rbegin(filename));

	size_t loaded_counter{};
	std::vector<char> content(constants::content_buffer_initial_size);
	for (const auto &[type, extension] : constants::shader_extensions) {
		std::copy_n(std::begin(extension), std::size(extension),
			std::next(std::begin(filename), name_length));

		if (load_file_to(content, filename)) {
			m_shaders.insert_or_assign(type, make_shader(filename, content));
			++loaded_counter;
		}
	}
	return loaded_counter;
}

VkShaderModule pipeline::make_shader(const std::string_view filename, const std::vector<char> &code) {
	const VkShaderModuleCreateInfo create_info{
		.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = static_cast<u32>(std::size(code)),
		.pCode    = reinterpret_cast<const u32 *>(std::data(code))
	};

	VkShaderModule shader;
	if (VK_SUCCESS != vkCreateShaderModule(m_device.handle(), &create_info, nullptr, &shader)) {
		throw pipeline_error{ fmt::format(
			R"(Failed to create shader module from "%s" file.)", filename
		) };
	}
	return shader;
}

bool pipeline::load_file_to(std::vector<char> &buffer, const std::string_view filename) {
	if (std::ifstream file{ std::data(filename), std::ios::ate | std::ios::binary }; file.is_open()) {
		buffer.resize(static_cast<size_t>(file.tellg()));
		file.seekg(std::ios::beg);
		file.read(std::data(buffer), std::size(buffer));
		return std::size(buffer) > 0;
	}
	return false;
}

#pragma endregion pipeline

#pragma region pipeline_layout

pipeline_layout::pipeline_layout(device &dev,
	const std::span<const VkDescriptorSetLayout> set_layouts,
	const std::span<const VkPushConstantRange> constant_ranges
) : m_device{ dev } {
	const VkPipelineLayoutCreateInfo layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount         = static_cast<u32>(std::size(set_layouts)),
		.pSetLayouts            = std::data(set_layouts),
		.pushConstantRangeCount = static_cast<u32>(std::size(constant_ranges)),
		.pPushConstantRanges    = std::data(constant_ranges)
	};

	if (VK_SUCCESS != vkCreatePipelineLayout(m_device.handle(), &layout_info, nullptr, &m_layout)) {
		throw pipeline_error{ "Failed to create pipeline layout." };
	}
}

pipeline_layout::~pipeline_layout() {
	vkDestroyPipelineLayout(m_device.handle(), m_layout, nullptr);
}

#pragma endregion pipeline_layout

} // namespace vc::engine::graphics
