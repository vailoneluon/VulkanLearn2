#include "pch.h"
#include "GeometryPass.h"
#include "Core/VulkanDescriptor.h"
#include "Scene/Model.h"

GeometryPass::GeometryPass(const GeometryPassCreateInfo& geometryInfo):
	m_TextureDescriptors(geometryInfo.textureManager->getDescriptor()),
	m_MeshManager(geometryInfo.meshManager),
	m_UboDescriptors(geometryInfo.uboDescriptors),
	m_ColorImages(geometryInfo.colorImages),
	m_DepthStencilImages(geometryInfo.depthStencilImages),
	m_OutputImage(geometryInfo.outputImage),
	m_BackgroundColor(geometryInfo.BackgroundColor), 
	m_SwapchainExtent(geometryInfo.vulkanSwapchainHandles->swapChainExtent),
	m_RenderObjects(geometryInfo.renderObjects),
	m_PushConstantData(geometryInfo.pushConstantData),
	m_VulkanHandles(geometryInfo.vulkanHandles)
{
	CreateDescriptor();
	CreatePipeline(geometryInfo);
}

GeometryPass::~GeometryPass()
{
	delete(m_Handles.pipeline);
}

void GeometryPass::Execute(VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
	CmdDrawScene(*cmdBuffer, currentFrame);
}

void GeometryPass::CreateDescriptor()
{
	m_Handles.descriptors.push_back(m_UboDescriptors->at(0));
	m_Handles.descriptors.push_back(m_TextureDescriptors);
}

void GeometryPass::CreatePipeline(const GeometryPassCreateInfo& geometryInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = geometryInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = geometryInfo.vulkanHandles;
	pipelineInfo.useVertexInput = true;
	pipelineInfo.swapchainHandles = geometryInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = geometryInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = geometryInfo.vertShaderFilePath;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

void GeometryPass::BindDescriptors(VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(
		*cmdBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		m_TextureDescriptors->getSetIndex(), 1,
		&m_TextureDescriptors->getHandles().descriptorSet,
		0, nullptr
	);

	vkCmdBindDescriptorSets(
		*cmdBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		(*m_UboDescriptors)[currentFrame]->getSetIndex(), 1,
		&(*m_UboDescriptors)[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

void GeometryPass::CmdDrawScene(VkCommandBuffer cmdBuffer, uint32_t currentFrame)
{
	// Transition Layout trước khi BeginRendering vì không còn Dependency quản lý.
	VulkanImage::TransitionLayout(
		cmdBuffer, (*m_ColorImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 0);

	VulkanImage::TransitionLayout(
		cmdBuffer, (*m_DepthStencilImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		0, 0);

	VulkanImage::TransitionLayout(
		cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 0);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_ColorImages)[currentFrame]->GetHandles().imageView;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// Resolve Msaa 
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveImageView = (*m_OutputImage)[currentFrame]->GetHandles().imageView;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;

	VkRenderingAttachmentInfo depthStencilAttachment{};
	depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };
	depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthStencilAttachment.imageView = (*m_DepthStencilImages)[currentFrame]->GetHandles().imageView;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Rendering Info dùng để bắt đầu dynamic rendering - thay thế vkcmdBeginRenderPass
	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;
	renderingInfo.pStencilAttachment = &depthStencilAttachment;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea.extent = m_SwapchainExtent;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.viewMask = 0;

	vkCmdBeginRendering(cmdBuffer, &renderingInfo);


	// Bind pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);

	// Bind global vertex/index buffer
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_MeshManager->getVertexBuffer(), &offset);
	vkCmdBindIndexBuffer(cmdBuffer, m_MeshManager->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	BindDescriptors(&cmdBuffer, currentFrame);

	// Ghi lệnh vẽ
	DrawSceneObject(cmdBuffer);

	vkCmdEndRendering(cmdBuffer);

	VulkanImage::TransitionLayout(
		cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 0);
}

void GeometryPass::DrawSceneObject(VkCommandBuffer cmdBuffer)
{
	for (const auto& renderObject : *m_RenderObjects)
	{
		std::vector<Mesh*> meshes = renderObject->getHandles().model->getMeshes();
		for (const auto& mesh : meshes)
		{
			// Cập nhật push constants với ma trận model và texture ID
			m_PushConstantData->model = renderObject->GetModelMatrix();
			m_PushConstantData->textureId = mesh->textureId;

			vkCmdPushConstants(cmdBuffer, m_Handles.pipeline->getHandles().pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_PushConstantData), &m_PushConstantData);

			// Vẽ mesh
			vkCmdDrawIndexed(cmdBuffer, mesh->meshRange.indexCount, 1, mesh->meshRange.firstIndex, mesh->meshRange.firstVertex, 0);
		}
	}
}
