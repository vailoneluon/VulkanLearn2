#include "pch.h"
#include "ShadowMapPass.h"
#include "Core\VulkanPipeline.h"
#include "Core\VulkanImage.h"
#include "Scene/MeshManager.h"
#include "Scene\RenderObject.h"
#include "Scene\Model.h"

ShadowMapPass::ShadowMapPass(const ShadowMapPassCreateInfo& shadowInfo):
	m_VulkanHandles(shadowInfo.vulkanHandles),
	m_MeshManager(shadowInfo.meshManager),
	m_RenderObjects(shadowInfo.renderObjects),
	m_BackgroundColor(shadowInfo.BackgroundColor),
	m_LightManager(shadowInfo.lightManager)
{
	CreatePipeline(shadowInfo);
}

ShadowMapPass::~ShadowMapPass()
{
	delete(m_Handles.pipeline);
}

void ShadowMapPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	for (const auto& light : m_LightManager->GetAllGpuLights(currentFrame))
	{
		if (light.params.z != -1)
		{
			VulkanImage::TransitionLayout(
				*cmdBuffer, m_LightManager->GetShadowMappingImage(currentFrame)[light.params.z]->GetHandles().image, 1,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				0, 1);

			VkRenderingAttachmentInfo depthAttachment{};
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depthAttachment.imageView = m_LightManager->GetShadowMappingImage(currentFrame)[light.params.z]->GetHandles().imageView;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			VkRenderingInfo renderingInfo{};
			renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			renderingInfo.colorAttachmentCount = 0;
			renderingInfo.pColorAttachments = nullptr;
			renderingInfo.pDepthAttachment = &depthAttachment;
			renderingInfo.pStencilAttachment = nullptr;
			renderingInfo.layerCount = 1;
			renderingInfo.renderArea.extent = { m_LightManager->GetShadowSize(), m_LightManager->GetShadowSize() };
			renderingInfo.renderArea.offset = { 0, 0 };

			vkCmdBeginRendering(*cmdBuffer, &renderingInfo);
			
			vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(*cmdBuffer, 0, 1, &m_MeshManager->getVertexBuffer(), &offset);
			vkCmdBindIndexBuffer(*cmdBuffer, m_MeshManager->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			DrawSceneObject(*cmdBuffer, light);

			vkCmdEndRendering(*cmdBuffer);

			VulkanImage::TransitionLayout(
				*cmdBuffer, m_LightManager->GetShadowMappingImage(currentFrame)[light.params.z]->GetHandles().image, 1,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				0, 1);
		}
	}
}

void ShadowMapPass::CreatePipeline(const ShadowMapPassCreateInfo& shadowMapInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.cullingMode = VK_CULL_MODE_FRONT_BIT;
	pipelineInfo.depthFormat = VK_FORMAT_D32_SFLOAT;
	pipelineInfo.stencilFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.enableDepthBias = true;
	pipelineInfo.fragmentShaderFilePath = shadowMapInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = shadowMapInfo.vertShaderFilePath;
	pipelineInfo.msaaSamples = shadowMapInfo.MSAA_SAMPLES;

	pipelineInfo.renderingColorAttachments = nullptr;

	pipelineInfo.swapchainHandles = shadowMapInfo.vulkanSwapchainHandles;
	pipelineInfo.useVertexInput = true;
	pipelineInfo.vulkanHandles = shadowMapInfo.vulkanHandles;
	pipelineInfo.viewportExtent = { m_LightManager->GetShadowSize(), m_LightManager->GetShadowSize() };

	pipelineInfo.pushConstantDataSize = sizeof(ShadowMapPushConstantData);

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

void ShadowMapPass::DrawSceneObject(VkCommandBuffer cmdBuffer, const GPULight& currentLight)
{
	for (const auto& renderObject : *m_RenderObjects)
	{
		std::vector<Mesh*> meshes = renderObject->getHandles().model->getMeshes();
		for (const auto& mesh : meshes)
		{
			
			m_PushConstantData.model = renderObject->GetModelMatrix();
			m_PushConstantData.lightMatrix = currentLight.lightSpaceMatrix;

			vkCmdPushConstants(cmdBuffer, m_Handles.pipeline->getHandles().pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &m_PushConstantData);

			// --- Ghi Lệnh Vẽ ---
			// Vẽ mesh hiện tại bằng cách sử dụng các offset trong buffer chung.
			vkCmdDrawIndexed(cmdBuffer, mesh->meshRange.indexCount, 1, mesh->meshRange.firstIndex, mesh->meshRange.firstVertex, 0);
		}
	}
}
