#include "pch.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"

VulkanRenderPass::VulkanRenderPass(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandles, VkSampleCountFlagBits msaaSamples) :
	m_VulkanHandles(vulkanHandles), m_SwapchainHandles(swapchainHandles), m_msaaSamples(msaaSamples)
{
	CreateRttRenderPass();
	CreateMainRenderPass();
}

VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyRenderPass(m_VulkanHandles.device, m_Handles.mainRenderPass, nullptr);
	vkDestroyRenderPass(m_VulkanHandles.device, m_Handles.rttRenderPass, nullptr);
}

void VulkanRenderPass::CreateRttRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	colorAttachment.samples = m_msaaSamples;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentDescription depthStencilAttachment{};
	depthStencilAttachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // Định dạng phổ biến cho depth/stencil
	depthStencilAttachment.samples = m_msaaSamples;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Không cần lưu lại depth buffer sau khi render
	depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef{};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription rttSubpassDesc{};
	rttSubpassDesc.colorAttachmentCount = 1;
	rttSubpassDesc.pColorAttachments = &colorAttachmentRef;
	rttSubpassDesc.pDepthStencilAttachment = &depthStencilAttachmentRef;

	VkAttachmentDescription attachments[] = { colorAttachment, depthStencilAttachment };

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Các hoạt động bên ngoài render pass
	dependency.dstSubpass = 0; // Subpass đầu tiên của chúng ta
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	VkRenderPassCreateInfo rttInfo{};
	rttInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rttInfo.attachmentCount = 2;
	rttInfo.pAttachments = attachments;
	rttInfo.dependencyCount = 1;
	rttInfo.pDependencies = &dependency;
	rttInfo.subpassCount = 1;
	rttInfo.pSubpasses = &rttSubpassDesc;

	VK_CHECK(vkCreateRenderPass(m_VulkanHandles.device, &rttInfo, nullptr, &m_Handles.rttRenderPass), "FAILED TO CREATE RTT RENDER PASS");
}

void VulkanRenderPass::CreateMainRenderPass()
{
	// --- Định nghĩa các Attachment ---

	// 1. Color Attachment (cho MSAA)
	// Đây là attachment chính để vẽ, trước khi được resolve ra swapchain image.
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format; // Định dạng phải khớp với swapchain
	colorAttachment.samples = m_msaaSamples; // Số lượng sample cho MSAA
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Xóa nội dung attachment trước khi render
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Lưu kết quả render
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Không quan tâm đến stencil
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout ban đầu không xác định
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout cuối cùng để chuẩn bị cho resolve

	// 2. Depth/Stencil Attachment
	VkAttachmentDescription depthStencilAttachment{};
	depthStencilAttachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // Định dạng phổ biến cho depth/stencil
	depthStencilAttachment.samples = m_msaaSamples;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Không cần lưu lại depth buffer sau khi render
	depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// 3. Color Attachment Resolve
	// Đây là attachment cuối cùng, là một image từ swapchain. Nội dung từ color attachment (MSAA) sẽ được resolve vào đây.
	VkAttachmentDescription resolveAttachment{};
	resolveAttachment.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Resolve target luôn có 1 sample
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout cuối cùng để chuẩn bị cho việc trình chiếu

	// --- Định nghĩa các Attachment Reference ---
	// Các reference này được sử dụng trong subpass để tham chiếu đến các attachment đã định nghĩa ở trên.

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // Index trong mảng pAttachments
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef{};
	depthStencilAttachmentRef.attachment = 1; // Index trong mảng pAttachments
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveAttachmentRef{};
	resolveAttachmentRef.attachment = 2; // Index trong mảng pAttachments
	resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// --- Định nghĩa Subpass ---
	// Một render pass có thể có nhiều subpass, nhưng ở đây ta chỉ dùng 1.
	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentRef;
	subpassDesc.pDepthStencilAttachment = &depthStencilAttachmentRef;
	subpassDesc.pResolveAttachments = &resolveAttachmentRef; // Trỏ đến resolve attachment

	// --- Định nghĩa Subpass Dependency ---
	// Dependency đảm bảo các giai đoạn pipeline được thực thi đúng thứ tự.
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Các hoạt động bên ngoài render pass
	dependency.dstSubpass = 0; // Subpass đầu tiên của chúng ta
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// --- Tạo Render Pass ---
	VkAttachmentDescription attachments[] = { colorAttachment, depthStencilAttachment, resolveAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 3;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(m_VulkanHandles.device, &renderPassInfo, nullptr, &m_Handles.mainRenderPass), "LỖI: Tạo render pass thất bại!");
}
