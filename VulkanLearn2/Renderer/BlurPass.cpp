#include "pch.h"
#include "BlurPass.h"

#include "Core/VulkanContext.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanDescriptor.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanImage.h"

BlurPass::BlurPass(const BlurPassCreateInfo& blurInfo) :
	m_VulkanHandles(blurInfo.vulkanHandles),
	m_SwapchainExtent(blurInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(blurInfo.BackgroundColor),
	m_OutputImages(blurInfo.outputImages)
{
	// Khởi tạo các tài nguyên cần thiết cho pass.
	CreateDescriptor(*blurInfo.inputTextures, blurInfo.vulkanSampler);
	CreatePipeline(blurInfo);
}

BlurPass::~BlurPass()
{
	// Dọn dẹp pipeline đã tạo.
	// Các descriptor sẽ được quản lý và dọn dẹp bởi VulkanDescriptorManager.
	delete(m_Handles.pipeline);
}

void BlurPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// --- 1. Chuyển đổi Layout cho Ảnh Đầu Ra ---
	// Chuyển layout của ảnh đầu ra từ UNDEFINED sang COLOR_ATTACHMENT_OPTIMAL để có thể ghi vào.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// --- 2. Thiết lập và Bắt đầu Dynamic Rendering ---
	// Cấu hình attachment màu cho việc render.
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_OutputImages)[currentFrame]->GetHandles().imageView;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Xóa sạch ảnh trước khi vẽ.
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Lưu kết quả sau khi vẽ.

	// Cấu hình thông tin cho dynamic rendering.
	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = nullptr; // Không dùng depth buffer.
	renderingInfo.pStencilAttachment = nullptr;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea.extent = m_SwapchainExtent;
	renderingInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRendering(*cmdBuffer, &renderingInfo);

	// --- 3. Thực hiện Vẽ ---
	vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
	BindDescriptors(cmdBuffer, currentFrame);
	DrawQuad(cmdBuffer); // Vẽ một quad toàn màn hình để kích hoạt fragment shader.

	vkCmdEndRendering(*cmdBuffer);

	// --- 4. Chuyển đổi Layout sau khi Render ---
	// Chuyển layout của ảnh đầu ra sang SHADER_READ_ONLY_OPTIMAL để nó có thể được dùng làm texture trong các pass tiếp theo.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 1);
}

/**
 * @brief Tạo descriptor set cho ảnh đầu vào.
 * Mỗi frame-in-flight sẽ có một descriptor set riêng, trỏ đến ảnh đầu vào tương ứng của frame đó.
 */
void BlurPass::CreateDescriptor(const std::vector<VulkanImage*>& inputTextures, const VulkanSampler* vulkanSampler)
{
	m_TextureDescriptors.resize(inputTextures.size());
	for (size_t i = 0; i < inputTextures.size(); i++)
	{
		// --- Binding 0: Input Texture ---
		// Cấu hình cho một binding duy nhất trong set, là một combined image sampler.
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Chỉ dùng trong fragment shader.

		// Thông tin về ảnh và sampler sẽ được bind.
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = inputTextures[i]->GetHandles().imageView;
		imageInfo.sampler = vulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		std::vector<BindingElementInfo> bindingElements = { bindingElement };

		// Tạo đối tượng VulkanDescriptor và thêm vào danh sách quản lý.
		m_TextureDescriptors[i] = new VulkanDescriptor(*m_VulkanHandles, bindingElements, 0); // Set 0
		m_Handles.descriptors.push_back(m_TextureDescriptors[i]);
	}
}

/**
 * @brief Tạo pipeline đồ họa cho BlurPass.
 * Pipeline này không có vertex input và được cấu hình để vẽ một quad toàn màn hình.
 */
void BlurPass::CreatePipeline(const BlurPassCreateInfo& blurInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = blurInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = blurInfo.vulkanHandles;
	pipelineInfo.useVertexInput = false; // Không cần vertex buffer, vì đỉnh được tạo trực tiếp trong vertex shader.
	pipelineInfo.swapchainHandles = blurInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = blurInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = blurInfo.vertShaderFilePath;
	pipelineInfo.depthFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.stencilFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.cullingMode = VK_CULL_MODE_NONE;

	std::vector<VkFormat> renderingColorAttachments = { VK_FORMAT_R16G16B16A16_SFLOAT };
	pipelineInfo.renderingColorAttachments = &renderingColorAttachments;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

/**
 * @brief Bind descriptor set của frame hiện tại vào command buffer.
 */
void BlurPass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_Handles.pipeline->getHandles().pipelineLayout,
		m_TextureDescriptors[currentFrame]->getSetIndex(), 1,
		&m_TextureDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

/**
 * @brief Ghi lệnh vẽ một quad toàn màn hình (2 tam giác, 6 đỉnh).
 */
void BlurPass::DrawQuad(const VkCommandBuffer* cmdBuffer)
{
	vkCmdDraw(*cmdBuffer, 6, 1, 0, 0);
}
