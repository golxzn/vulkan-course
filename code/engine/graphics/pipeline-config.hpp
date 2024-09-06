#pragma once

#include <glm/vec2.hpp>
#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace vc::engine::graphics {

struct pipeline_config {
	VkViewport viewport{
		.x        = 0.0f, .y        = 0.0f,
		.width    = 0.0f, .height   = 0.0f,
		.minDepth = 0.0f, .maxDepth = 1.0f,
	};
	VkRect2D   scissor{
		.offset = VkOffset2D{ 0, 0 },
		.extent = VkExtent2D{ 0, 0 }
	};
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{
		.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
	VkPipelineRasterizationStateCreateInfo rasterization_create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable        = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = VK_POLYGON_MODE_FILL,
		.cullMode                = VK_CULL_MODE_NONE,
		.frontFace               = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable         = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp          = 0.0f,
		.depthBiasSlopeFactor    = 0.0f,
		.lineWidth               = 1.0f,
	};
	VkPipelineMultisampleStateCreateInfo multi_sample_create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable   = VK_FALSE,
		.minSampleShading      = 1.0f,
		.pSampleMask           = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable      = VK_FALSE
	};
	VkPipelineColorBlendAttachmentState color_attachment_state{
		.blendEnable         = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp        = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp        = VK_BLEND_OP_ADD,
		.colorWriteMask
			= VK_COLOR_COMPONENT_R_BIT
			| VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT
			| VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo color_blend_create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable   = VK_FALSE,
		.logicOp         = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments    = &color_attachment_state,
		.blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f }
	};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable       = VK_TRUE,
		.depthWriteEnable      = VK_TRUE,
		.depthCompareOp        = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable     = VK_FALSE,
		.front                 = {},
		.back                  = {},
		.minDepthBounds        = 0.0f,
		.maxDepthBounds        = 1.0f
	};
	VkPipelineLayout layout     { VK_NULL_HANDLE };
	VkRenderPass     render_pass{ VK_NULL_HANDLE };
	u32              sub_pass   { 0 };
};


} // namespace vc::engine::graphics
