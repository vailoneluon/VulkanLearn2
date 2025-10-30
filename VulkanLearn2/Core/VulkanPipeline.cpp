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
	std::vector<VulkanDescriptor*>& descriptors)
	: m_VulkanHandles(vulkanHandles)
{
	// Tạo các shader module từ file SPIR-V đã được biên dịch.
	VkShaderModule vertShaderModule = CreateShaderModule("Shaders/vert.spv");
	VkShaderModule fragShaderModule = CreateShaderModule("Shaders/frag.spv");

	// Tạo Pipeline Layout, định nghĩa các descriptor set và push constant mà pipeline sẽ sử dụng.
	CreatePipelineLayout(descriptors);

	// Tạo Graphics Pipeline với tất cả các cấu hình cần thiết.
	CreateGraphicsPipeline(
		renderPassHandles,
		swapchainHandles,
		msaaSamples,
		vertShaderModule,
		fragShaderModule
	);

	// Sau khi pipeline đã được tạo, các shader module không còn cần thiết và có thể được hủy.
	vkDestroyShaderModule(m_VulkanHandles.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_VulkanHandles.device, vertShaderModule, nullptr);
}

VulkanPipeline::~VulkanPipeline()
{
	// Hủy pipeline và layout của nó.
	vkDestroyPipeline(m_VulkanHandles.device, m_Handles.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_VulkanHandles.device, m_Handles.pipelineLayout, nullptr);
}

std::vector<char> VulkanPipeline::ReadShaderFile(const std::string& filename)
{
	// Mở file ở chế độ nhị phân và con trỏ đặt ở cuối file.
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("LỖI: Không thể mở file shader: " + filename);
	}

	// Lấy kích thước file từ vị trí con trỏ.
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	// Quay lại đầu file và đọc toàn bộ nội dung.
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule VulkanPipeline::CreateShaderModule(const std::string& shaderFilePath)
{
	std::vector<char> code = ReadShaderFile(shaderFilePath);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	VK_CHECK(vkCreateShaderModule(m_VulkanHandles.device, &createInfo, nullptr, &shaderModule), "LỖI: Tạo shader module thất bại!");
	return shaderModule;
}

void VulkanPipeline::CreatePipelineLayout(const std::vector<VulkanDescriptor*>& descriptors)
{
	// Sắp xếp các descriptor set layout theo chỉ số set (set index) để đảm bảo thứ tự đúng.
	std::map<uint32_t, VkDescriptorSetLayout> sortedLayouts;
	for (const auto* descriptor : descriptors)
	{
		sortedLayouts[descriptor->getSetIndex()] = descriptor->getHandles().descriptorSetLayout;
	}

	std::vector<VkDescriptorSetLayout> descSetLayouts;
	descSetLayouts.reserve(sortedLayouts.size());
	for (const auto& pair : sortedLayouts)
	{
		descSetLayouts.push_back(pair.second);
	}

	// Định nghĩa một dải push constant để truyền ma trận model vào vertex shader.
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantData);

	// Tạo Pipeline Layout.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descSetLayouts.size()); 
	pipelineLayoutInfo.pSetLayouts = descSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK(vkCreatePipelineLayout(m_VulkanHandles.device, &pipelineLayoutInfo, nullptr, &m_Handles.pipelineLayout), "LỖI: Tạo pipeline layout thất bại!");
}

void VulkanPipeline::CreateGraphicsPipeline(
	const RenderPassHandles& renderPassHandles,
	const SwapchainHandles& swapchainHandles,
	VkSampleCountFlagBits msaaSamples,
	VkShaderModule vertShaderModule,
	VkShaderModule fragShaderModule)
{
	// 1. Giai đoạn Shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main"; // Tên hàm entry point trong shader

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// 2. Giai đoạn Vertex Input: Mô tả định dạng dữ liệu vertex đầu vào.
	auto vertexBindingDescs = Vertex::GetBindingDesc();
	auto vertexAttributeDescs = Vertex::GetAttributeDesc();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescs.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescs.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescs.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescs.data();

	// 3. Giai đoạn Input Assembly: Mô tả cách các vertex được lắp ráp thành primitive (vd: tam giác).
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// 4. Giai đoạn Viewport và Scissor: Định nghĩa vùng không gian render.
	// Khai báo tĩnh, kích thước viewport và scissor sẽ được cố định khi tạo pipeline.
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

	// 5. Giai đoạn Rasterization: Chuyển đổi primitive thành fragment.
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE; // Tạm thời không cull mặt nào
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// 6. Giai đoạn Multisampling: Cấu hình cho MSAA.
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;

	// 7. Giai đoạn Depth/Stencil: Cấu hình kiểm tra chiều sâu.
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// 8. Giai đoạn Color Blending: Cấu hình cách màu sắc được ghi vào framebuffer.
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	// 9. Dynamic State: Không sử dụng dynamic state cho viewport và scissor nữa.
	// pDynamicState được set là nullptr.

	// 10. Tổng hợp và tạo Graphics Pipeline
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
	pipelineInfo.layout = m_Handles.pipelineLayout;
	pipelineInfo.renderPass = renderPassHandles.renderPass;
	pipelineInfo.subpass = 0;

	VK_CHECK(vkCreateGraphicsPipelines(m_VulkanHandles.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Handles.graphicsPipeline), "LỖI: Tạo graphics pipeline thất bại!");
}