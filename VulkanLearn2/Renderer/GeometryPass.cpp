#include "pch.h"
#include "GeometryPass.h"
#include "Core/VulkanDescriptor.h"
#include "Scene/Model.h"

#include "Core/VulkanContext.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanPipeline.h"
#include "Scene/TextureManager.h"
#include "Scene/MeshManager.h"
#include "Scene/RenderObject.h"
#include "Core/VulkanImage.h"
#include "Scene\MaterialManager.h"


GeometryPass::GeometryPass(const GeometryPassCreateInfo& geometryInfo) :
	m_TextureDescriptors(geometryInfo.textureManager->getDescriptor()),
	m_MeshManager(geometryInfo.meshManager),
	m_MaterialManager(geometryInfo.materialManager),
	m_DepthStencilImages(geometryInfo.depthStencilImages),
	m_BackgroundColor(geometryInfo.BackgroundColor),
	m_SwapchainExtent(geometryInfo.vulkanSwapchainHandles->swapChainExtent),
	m_RenderObjects(geometryInfo.renderObjects),
	m_PushConstantData(geometryInfo.pushConstantData),
	m_VulkanHandles(geometryInfo.vulkanHandles),
	m_AlbedoImages(geometryInfo.albedoImages),
	m_NormalImages(geometryInfo.normalImages),
	m_PositionImages(geometryInfo.positionImages)
{
	CreateDescriptor(*geometryInfo.uniformBuffers);
	CreatePipeline(geometryInfo);
}

GeometryPass::~GeometryPass()
{
	delete(m_Handles.pipeline);
}

void GeometryPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// --- 1. Chuyển đổi Layout cho các Attachment ---


	// Chuyển layout cho attachment depth/stencil (MSAA) để có thể ghi vào.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_DepthStencilImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		0, 1);


	// G-Buffer
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_AlbedoImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_NormalImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_PositionImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// --- 2. Thiết lập và Bắt đầu Dynamic Rendering ---
	// Cấu hình attachment màu, bao gồm cả việc resolve MSAA.
	VkRenderingAttachmentInfo albedoAttachment{};
	albedoAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	albedoAttachment.clearValue.color = m_BackgroundColor;
	albedoAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	albedoAttachment.imageView = (*m_AlbedoImages)[currentFrame]->GetHandles().imageView; // Vẽ vào ảnh MSAA.
	albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// Normal
	VkRenderingAttachmentInfo normalAttachment{};
	normalAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	normalAttachment.clearValue.color = m_BackgroundColor;
	normalAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normalAttachment.imageView = (*m_NormalImages)[currentFrame]->GetHandles().imageView; 
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// Position
	VkRenderingAttachmentInfo positionAttachment{};
	positionAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	positionAttachment.clearValue.color = m_BackgroundColor;
	positionAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	positionAttachment.imageView = (*m_PositionImages)[currentFrame]->GetHandles().imageView;
	positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// Cấu hình attachment depth/stencil.
	VkRenderingAttachmentInfo depthStencilAttachment{};
	depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };
	depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthStencilAttachment.imageView = (*m_DepthStencilImages)[currentFrame]->GetHandles().imageView;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkRenderingAttachmentInfo attachments[] = { albedoAttachment, normalAttachment, positionAttachment };

	// Cấu hình thông tin cho dynamic rendering.
	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 3;
	renderingInfo.pColorAttachments = attachments;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;
	renderingInfo.pStencilAttachment = &depthStencilAttachment;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea.extent = m_SwapchainExtent;
	renderingInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRendering(*cmdBuffer, &renderingInfo);

	// --- 3. Thực hiện Vẽ ---
	// Bind pipeline và các tài nguyên chung.
	vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(*cmdBuffer, 0, 1, &m_MeshManager->getVertexBuffer(), &offset);
	vkCmdBindIndexBuffer(*cmdBuffer, m_MeshManager->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
	BindDescriptors(cmdBuffer, currentFrame);

	// Vẽ tất cả các đối tượng trong scene.
	DrawSceneObject(*cmdBuffer);

	vkCmdEndRendering(*cmdBuffer);

	// --- 4. Chuyển đổi Layout sau khi Render ---
	
	// G-Buffer
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_AlbedoImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 1);

	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_NormalImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 1);

	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_PositionImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 1);
}

/**
 * @brief Tạo các descriptor set cho GeometryPass.
 * Pass này sử dụng hai set:
 * - Set 0: Chứa mảng các texture (từ TextureManager).
 * - Set 1: Chứa Uniform Buffer Object (UBO) với thông tin camera.
 */
void GeometryPass::CreateDescriptor(const std::vector<VulkanBuffer*>& uniformBuffers)
{
	// Set 0: Texture array descriptor (được tạo và quản lý bởi TextureManager).
	m_Handles.descriptors.push_back(m_TextureDescriptors);

	// Set 1: UBO descriptor (một cho mỗi frame-in-flight).
	m_UboDescriptors.resize(uniformBuffers.size());
	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{
		// Cấu hình binding cho UBO.
		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0; // layout(binding = 0) trong set 1.
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Dùng trong Vertex Shader.
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
		m_UboDescriptors[i] = new VulkanDescriptor(*m_VulkanHandles, uniformBindings, 1); // Set 1
		m_Handles.descriptors.push_back(m_UboDescriptors[i]);
	}

	// Set 2: SBO Chứa thông tin Material
	m_Handles.descriptors.push_back(m_MaterialManager->GetDescriptor());
}

/**
 * @brief Tạo pipeline đồ họa cho GeometryPass.
 * Pipeline này được cấu hình để nhận dữ liệu vertex đầu vào.
 */
void GeometryPass::CreatePipeline(const GeometryPassCreateInfo& geometryInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = geometryInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = geometryInfo.vulkanHandles;
	pipelineInfo.useVertexInput = true; // Quan trọng: Pass này xử lý dữ liệu vertex thực tế.
	pipelineInfo.swapchainHandles = geometryInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = geometryInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = geometryInfo.vertShaderFilePath;

	std::vector<VkFormat> renderingColorAttachments = { 
		VK_FORMAT_B8G8R8A8_SRGB,					// Albedo
		VK_FORMAT_R16G16B16A16_SFLOAT,			// Normal
		VK_FORMAT_R16G16B16A16_SFLOAT			// Position
	};
	pipelineInfo.renderingColorAttachments = &renderingColorAttachments;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

/**
 * @brief Bind các descriptor set cần thiết cho GeometryPass.
 */
void GeometryPass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	// Bind Set 0: Mảng các texture.
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		m_TextureDescriptors->getSetIndex(), 1,
		&m_TextureDescriptors->getHandles().descriptorSet,
		0, nullptr
	);

	// Bind Set 1: UBO chứa thông tin camera cho frame hiện tại.
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		m_UboDescriptors[currentFrame]->getSetIndex(), 1,
		&m_UboDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);

	// Bind Set 2: SBO chứa thông tin Material.
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		m_MaterialManager->GetDescriptor()->getSetIndex(), 1,
		&m_MaterialManager->GetDescriptor()->getHandles().descriptorSet,
		0, nullptr
	);
}

/**
 * @brief Ghi lệnh vẽ cho tất cả các đối tượng trong scene.
 * Duyệt qua từng đối tượng và từng mesh của đối tượng đó để vẽ.
 */
void GeometryPass::DrawSceneObject(VkCommandBuffer cmdBuffer)
{
	for (const auto& renderObject : *m_RenderObjects)
	{
		std::vector<Mesh*> meshes = renderObject->getHandles().model->getMeshes();
		for (const auto& mesh : meshes)
		{
			// --- Cập nhật Push Constants ---
			// Gửi dữ liệu cho từng lần vẽ (per-draw data) như ma trận model và ID texture.
			// Đây là cách hiệu quả để gửi một lượng nhỏ dữ liệu thay đổi thường xuyên.
			m_PushConstantData->model = renderObject->GetModelMatrix();
			m_PushConstantData->materialIndex = mesh->materialIndex;

			vkCmdPushConstants(cmdBuffer, m_Handles.pipeline->getHandles().pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), m_PushConstantData);

			// --- Ghi Lệnh Vẽ ---
			// Vẽ mesh hiện tại bằng cách sử dụng các offset trong buffer chung.
			vkCmdDrawIndexed(cmdBuffer, mesh->meshRange.indexCount, 1, mesh->meshRange.firstIndex, mesh->meshRange.firstVertex, 0);
		}
	}
}
