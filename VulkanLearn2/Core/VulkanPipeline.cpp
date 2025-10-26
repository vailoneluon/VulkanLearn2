#include "pch.h"
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanDescriptor.h"

VulkanPipeline::VulkanPipeline(
	const VulkanHandles& vulkanHandles,
	const RenderPassHandles& renderPassHandles,
	const SwapchainHandles& swapchainHandles,
	VkSampleCountFlagBits msaaSamples,
	vector<VulkanDescriptor*>& descriptors)
	: vk(vulkanHandles)
{
	VkShaderModule vertShaderModule = createShaderModule("Shaders/vert.spv");
	VkShaderModule fragShaderModule = createShaderModule("Shaders/frag.spv");

	// Tạo Pipeline Layout 
	createPipelineLayout(descriptors);

	// Tạo Graphics Pipeline
	createGraphicsPipeline(
		renderPassHandles,
		swapchainHandles,
		msaaSamples,
		vertShaderModule,
		fragShaderModule
	);

	// Hủy shader module sau khi pipeline đã được tạo
	vkDestroyShaderModule(vk.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(vk.device, vertShaderModule, nullptr);
}

VulkanPipeline::~VulkanPipeline()
{
	vkDestroyPipeline(vk.device, handles.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(vk.device, handles.pipelineLayout, nullptr);
}

// Hàm đọc file (thường dùng cho shader)
vector<char> VulkanPipeline::readShaderFile(const string& filename)
{
	ifstream file(filename, ios::ate | ios::binary);

	if (!file.is_open())
	{
		showError("FAILED TO OPEN SHADER FILE: " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

// Hàm tạo Shader Module từ mã SPIR-V
VkShaderModule VulkanPipeline::createShaderModule(const string& shaderFilePath)
{
	vector<char> code = readShaderFile(shaderFilePath);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	VK_CHECK(vkCreateShaderModule(vk.device, &createInfo, nullptr, &shaderModule), "FAILED TO CREATE SHADER MODULE");
	return shaderModule;
}

// Tạo một Pipeline Layout 
void VulkanPipeline::createPipelineLayout(const vector<VulkanDescriptor*>& descriptors)
{
	// Sắp xếp layout theo thứ tự SetIndex.
	vector<VkDescriptorSetLayout> descSetLayouts;
	map<uint32_t, VkDescriptorSetLayout> descSetLayoutMaps;
	for (const auto* descriptor : descriptors)
	{
		descSetLayoutMaps[descriptor->getSetIndex()] = descriptor->getHandles().descriptorSetLayout;
	}
	for (const auto&[setIndex, descriptorSetLayout] : descSetLayoutMaps)
	{
		descSetLayouts.push_back(descriptorSetLayout);
	}

	// Tạo Pipeline Layout 
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descSetLayouts.size(); 
	pipelineLayoutInfo.pSetLayouts = descSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Không có push constant
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VK_CHECK(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &handles.pipelineLayout), "FAILED TO CREATE PIPELINE LAYOUT");
}

// Hàm tạo pipeline chính
void VulkanPipeline::createGraphicsPipeline(
	const RenderPassHandles& renderPassHandles,
	const SwapchainHandles& swapchainHandles,
	VkSampleCountFlagBits msaaSamples,
	VkShaderModule vertShaderModule,
	VkShaderModule fragShaderModule)
{
	// 1. Định nghĩa giai đoạn Shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Vertex Pipeline
	auto vertexBindingDescs = Vertex::GetBindingDesc();
	auto vertexAttributeDescs = Vertex::GetAttributeDesc();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescs;
	vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescs.size();
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescs.data();

	// 3. Input Assembly (Vẽ tam giác)
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// 4. Viewport và Scissor (Sử dụng kích thước của swapchain)
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchainHandles.swapChainExtent.width;
	viewport.height = (float)swapchainHandles.swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainHandles.swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// 5. Rasterizer (Tô đầy tam giác)
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// 6. Multisampling (Sử dụng MSAA từ class FrameBuffer)
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;

	// 7. Depth/Stencil (Bật kiểm tra độ sâu)
	// (Phù hợp với render pass của bạn)
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE; // Bật stencil test

	// 8. Color Blending (Ghi đè màu, không trộn)
	// (Phù hợp với color attachment và resolve attachment)
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	// 9. Tạo Pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Không có state động
	pipelineInfo.layout = handles.pipelineLayout;
	pipelineInfo.renderPass = renderPassHandles.renderPass; // Render pass
	pipelineInfo.subpass = 0; // Subpass đầu tiên

	VK_CHECK(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handles.graphicsPipeline), "FAILED TO CREATE GRAPHICS PIPELINE");
}