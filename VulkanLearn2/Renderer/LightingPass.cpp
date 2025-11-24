#include "pch.h"
#include "LightingPass.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanDescriptor.h"
#include "Core/VulkanBuffer.h"

LightingPass::LightingPass(const LightingPassCreateInfo& lightingInfo) :
	m_VulkanHandles(lightingInfo.vulkanHandles),
	m_SwapchainExtent(lightingInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(lightingInfo.BackgroundColor),
	m_OutputImages(lightingInfo.outputImages), 
	m_SceneLightDescriptors(*lightingInfo.sceneLightDescriptors)
{
	CreateDescriptor(
		*lightingInfo.gAlbedoTextures,
		*lightingInfo.gNormalTextures,
		*lightingInfo.gPositionTextures,
		lightingInfo.vulkanSampler,
		*lightingInfo.uniformBuffers);
	CreatePipeline(lightingInfo);
}

LightingPass::~LightingPass()
{
	delete(m_Handles.pipeline);
}

void LightingPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// --- 1. Chuyển đổi Layout cho Ảnh Đầu Ra ---
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// --- 2. Thiết lập và Bắt đầu Dynamic Rendering ---
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_OutputImages)[currentFrame]->GetHandles().imageView;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pStencilAttachment = nullptr;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea.extent = m_SwapchainExtent;
	renderingInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRendering(*cmdBuffer, &renderingInfo);

	// --- 3. Thực hiện Vẽ ---
	vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
	BindDescriptors(cmdBuffer, currentFrame);
	DrawQuad(*cmdBuffer);

	vkCmdEndRendering(*cmdBuffer);

	// --- 4. Chuyển đổi Layout sau khi Render ---
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 1);
}


void LightingPass::CreateDescriptor(
	const std::vector<VulkanImage*>& gAlbedoTextures,
	const std::vector<VulkanImage*>& gNormalTextures,
	const std::vector<VulkanImage*>& gPositionTextures,
 	const VulkanSampler* vulkanSampler,
	const std::vector<VulkanBuffer*>& uniformBuffers)
{
	// =================================================================================================
	// DESCRIPTOR SET 0: G-BUFFER INPUTS
	// =================================================================================================
	m_TextureDescriptors.resize(gAlbedoTextures.size());
	for (size_t i = 0; i < gAlbedoTextures.size(); i++)
	{
		// --- Binding 0: G-Buffer Albedo ---
		BindingElementInfo albedoBinding{};
		albedoBinding.binding = 0;
		albedoBinding.descriptorCount = 1;
		albedoBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo albedoImageInfo{};
		albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoImageInfo.imageView = gAlbedoTextures[i]->GetHandles().imageView;
		albedoImageInfo.sampler = vulkanSampler->getSampler();
		ImageDescriptorUpdateInfo albedoUpdateInfo{};
		albedoUpdateInfo.binding = 0;
		albedoUpdateInfo.firstArrayElement = 0;
		albedoUpdateInfo.imageInfos = { albedoImageInfo };
		albedoBinding.imageDescriptorUpdateInfoCount = 1;
		albedoBinding.pImageDescriptorUpdates = &albedoUpdateInfo;

		// --- Binding 1: G-Buffer Normal ---
		BindingElementInfo normalBinding{};
		normalBinding.binding = 1;
		normalBinding.descriptorCount = 1;
		normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo normalImageInfo{};
		normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalImageInfo.imageView = gNormalTextures[i]->GetHandles().imageView;
		normalImageInfo.sampler = vulkanSampler->getSampler();
		ImageDescriptorUpdateInfo normalUpdateInfo{};
		normalUpdateInfo.binding = 1;
		normalUpdateInfo.firstArrayElement = 0;
		normalUpdateInfo.imageInfos = { normalImageInfo };
		normalBinding.imageDescriptorUpdateInfoCount = 1;
		normalBinding.pImageDescriptorUpdates = &normalUpdateInfo;

		// --- Binding 2: G-Buffer Position ---
		BindingElementInfo positionBinding{};
		positionBinding.binding = 2;
		positionBinding.descriptorCount = 1;
		positionBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		positionBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo positionImageInfo{};
		positionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		positionImageInfo.imageView = gPositionTextures[i]->GetHandles().imageView;
		positionImageInfo.sampler = vulkanSampler->getSampler();
		ImageDescriptorUpdateInfo positionUpdateInfo{};
		positionUpdateInfo.binding = 2;
		positionUpdateInfo.firstArrayElement = 0;
		positionUpdateInfo.imageInfos = { positionImageInfo };
		positionBinding.imageDescriptorUpdateInfoCount = 1;
		positionBinding.pImageDescriptorUpdates = &positionUpdateInfo;


		// Tạo descriptor với cả 3 binding
		std::vector<BindingElementInfo> bindingElements = { albedoBinding, normalBinding, positionBinding };
		m_TextureDescriptors[i] = new VulkanDescriptor(*m_VulkanHandles, bindingElements, 0); // Set 0
		m_Handles.descriptors.push_back(m_TextureDescriptors[i]);
	}

	// =================================================================================================
	// DESCRIPTOR SET 1: SCENE LIGHTS
	// =================================================================================================
	m_Handles.descriptors.insert(m_Handles.descriptors.end(), m_SceneLightDescriptors.begin(), m_SceneLightDescriptors.end());

	// =================================================================================================
	// DESCRIPTOR SET 2: CAMERA UBO
	// =================================================================================================
	m_UboDescriptors.resize(uniformBuffers.size());
	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{
		// Cấu hình binding cho UBO.
		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0; // layout(binding = 0) trong set 2.
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // SỬA LỖI: Phải là FRAGMENT_BIT vì viewPos dùng trong fragment shader.
		uniformElementInfo.descriptorCount = 1;

		// Thông tin về buffer sẽ được bind.
		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = uniformBuffers[i]->GetHandles().buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = uniformBuffers[i]->GetHandles().bufferSize;

		std::vector<VkDescriptorBufferInfo> uniformBufferInfos = { uniformBufferInfo };

		BufferDescriptorUpdateInfo uniformBufferUpdate{};
		uniformBufferUpdate.binding = 0;
		uniformBufferUpdate.firstArrayElement = 0;
		uniformBufferUpdate.bufferInfos = uniformBufferInfos;

		uniformElementInfo.bufferDescriptorUpdateInfoCount = 1;
		uniformElementInfo.pBufferDescriptorUpdates = &uniformBufferUpdate;

		// Tạo đối tượng VulkanDescriptor và thêm vào danh sách quản lý.
		std::vector<BindingElementInfo> uniformBindings{ uniformElementInfo };
		m_UboDescriptors[i] = new VulkanDescriptor(*m_VulkanHandles, uniformBindings, 2); // Set 2
		m_Handles.descriptors.push_back(m_UboDescriptors[i]);
	}
}

void LightingPass::CreatePipeline(const LightingPassCreateInfo& lightingInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = lightingInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = lightingInfo.vulkanHandles;
	pipelineInfo.useVertexInput = false;
	pipelineInfo.swapchainHandles = lightingInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = lightingInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = lightingInfo.vertShaderFilePath;
	pipelineInfo.depthFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.stencilFormat = VK_FORMAT_UNDEFINED;

	std::vector<VkFormat> renderingColorAttachments = { VK_FORMAT_R16G16B16A16_SFLOAT };
	pipelineInfo.renderingColorAttachments = &renderingColorAttachments;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

void LightingPass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	// Bind Set 0: G-Buffer textures
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_Handles.pipeline->getHandles().pipelineLayout,
		m_TextureDescriptors[currentFrame]->getSetIndex(), 1,
		&m_TextureDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);

	// Bind Set 1: Scene Light SSBO
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_Handles.pipeline->getHandles().pipelineLayout,
		m_SceneLightDescriptors[currentFrame]->getSetIndex(), 1,
		&m_SceneLightDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);

	// SỬA LỖI: Bind Set 2: Camera UBO
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_Handles.pipeline->getHandles().pipelineLayout,
		m_UboDescriptors[currentFrame]->getSetIndex(), 1,
		&m_UboDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

void LightingPass::DrawQuad(VkCommandBuffer cmdBuffer)
{
	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}
