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
	m_ColorImages(geometryInfo.colorImages),
	m_DepthStencilImages(geometryInfo.depthStencilImages),
	m_OutputImage(geometryInfo.outputImage),
	m_BackgroundColor(geometryInfo.BackgroundColor),
	m_SwapchainExtent(geometryInfo.vulkanSwapchainHandles->swapChainExtent),
	m_RenderObjects(geometryInfo.renderObjects),
	m_PushConstantData(geometryInfo.pushConstantData),
	m_VulkanHandles(geometryInfo.vulkanHandles)
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
	// Chuyển layout cho attachment màu (MSAA) để có thể ghi vào.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_ColorImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// Chuyển layout cho attachment depth/stencil (MSAA) để có thể ghi vào.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_DepthStencilImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		0, 1);

	// Chuyển layout cho ảnh đầu ra (không MSAA) để nó có thể nhận kết quả resolve.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// --- 2. Thiết lập và Bắt đầu Dynamic Rendering ---
	// Cấu hình attachment màu, bao gồm cả việc resolve MSAA.
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_ColorImages)[currentFrame]->GetHandles().imageView; // Vẽ vào ảnh MSAA.
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Không cần lưu ảnh MSAA sau khi resolve.
	// Cấu hình resolve: kết quả từ ảnh MSAA sẽ được resolve và ghi vào ảnh output (không MSAA).
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveImageView = (*m_OutputImage)[currentFrame]->GetHandles().imageView;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;

	// Cấu hình attachment depth/stencil.
	VkRenderingAttachmentInfo depthStencilAttachment{};
	depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };
	depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthStencilAttachment.imageView = (*m_DepthStencilImages)[currentFrame]->GetHandles().imageView;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Cấu hình thông tin cho dynamic rendering.
	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
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
	// Chuyển layout của ảnh đầu ra sang SHADER_READ_ONLY_OPTIMAL để nó có thể được dùng làm texture trong các pass tiếp theo.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
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
