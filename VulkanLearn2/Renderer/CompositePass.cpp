#include "pch.h"
#include "CompositePass.h"

#include "Core/VulkanContext.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanDescriptor.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanImage.h"

CompositePass::CompositePass(const CompositePassCreateInfo& compositeInfo) :
	m_VulkanHandles(compositeInfo.vulkanHandles),
	m_VulkanSwapchainHandles(compositeInfo.vulkanSwapchainHandles),
	m_SwapchainExtent(compositeInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(compositeInfo.BackgroundColor),
	m_MainColorImage(compositeInfo.mainColorImage)
{
	// Xóa bỏ comment cũ không còn dùng
	// m_TextureDescriptors(compositeInfo.textureDescriptors),

	CreateDescriptor(*compositeInfo.inputTextures0, *compositeInfo.inputTextures1, compositeInfo.vulkanSampler);
	CreatePipeline(compositeInfo);
}

CompositePass::~CompositePass()
{
	delete(m_Handles.pipeline);
}

void CompositePass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// --- 1. Chuyển đổi Layout cho các Attachment ---
	// Chuyển layout của attachment màu (MSAA) để có thể ghi vào.
	VulkanImage::TransitionLayout(
		*cmdBuffer, m_MainColorImage->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// Chuyển layout của swapchain image để nó có thể nhận kết quả resolve từ attachment màu (MSAA).
	VulkanImage::TransitionLayout(
		*cmdBuffer, m_VulkanSwapchainHandles->swapchainImages[imageIndex], 1,
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
	colorAttachment.imageView = m_VulkanSwapchainHandles->swapchainImageViews[imageIndex];
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Không cần lưu ảnh MSAA sau khi resolve.

	// Cấu hình thông tin cho dynamic rendering.
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
	DrawQuad(cmdBuffer);

	vkCmdEndRendering(*cmdBuffer);

	// --- 4. Chuyển đổi Layout sau khi Render ---
	// Chuyển layout của swapchain image sang PRESENT_SRC_KHR để sẵn sàng cho việc trình chiếu lên màn hình.
	VulkanImage::TransitionLayout(
		*cmdBuffer, m_VulkanSwapchainHandles->swapchainImages[imageIndex], 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
		0, 1);
}

/**
 * @brief Tạo descriptor set cho hai ảnh đầu vào (scene và bloom).
 */
void CompositePass::CreateDescriptor(const std::vector<VulkanImage*>& inputTextures0, const std::vector<VulkanImage*>& inputTextures1, const VulkanSampler* vulkanSampler)
{
	m_TextureDescriptors.resize(inputTextures0.size());
	for (size_t i = 0; i < inputTextures0.size(); i++)
	{
		// --- Binding 0: Scene Texture ---
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = inputTextures0[i]->GetHandles().imageView;
		imageInfo.sampler = vulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos{ imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		// --- Binding 1: Bright (Bloom) Texture ---
		BindingElementInfo bindingElement2{};
		bindingElement2.binding = 1;
		bindingElement2.descriptorCount = 1;
		bindingElement2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo2{};
		imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo2.imageView = inputTextures1[i]->GetHandles().imageView;
		imageInfo2.sampler = vulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo2{};
		imageUpdateInfo2.binding = 1;
		imageUpdateInfo2.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos2{ imageInfo2 };
		imageUpdateInfo2.imageInfos = imageInfos2;

		bindingElement2.imageDescriptorUpdateInfoCount = 1;
		bindingElement2.pImageDescriptorUpdates = &imageUpdateInfo2;

		// Tạo descriptor với cả hai binding.
		std::vector<BindingElementInfo> bindingElements = { bindingElement, bindingElement2 };
		m_TextureDescriptors[i] = new VulkanDescriptor(*m_VulkanHandles, bindingElements, 0); // Set 0
		m_Handles.descriptors.push_back(m_TextureDescriptors[i]);
	}
}

/**
 * @brief Tạo pipeline đồ họa cho CompositePass.
 */
void CompositePass::CreatePipeline(const CompositePassCreateInfo& compositeInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = compositeInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = compositeInfo.vulkanHandles;
	pipelineInfo.useVertexInput = false;
	pipelineInfo.swapchainHandles = compositeInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = compositeInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = compositeInfo.vertShaderFilePath;
	pipelineInfo.depthFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.stencilFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.cullingMode = VK_CULL_MODE_NONE;

	std::vector<VkFormat> renderingColorAttachments = { VK_FORMAT_B8G8R8A8_SRGB };
	pipelineInfo.renderingColorAttachments = &renderingColorAttachments;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

/**
 * @brief Bind descriptor set của frame hiện tại vào command buffer.
 */
void CompositePass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
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
 * @brief Ghi lệnh vẽ một quad toàn màn hình.
 */
void CompositePass::DrawQuad(const VkCommandBuffer* cmdBuffer)
{
	vkCmdDraw(*cmdBuffer, 6, 1, 0, 0);
}
