#include "pch.h"
#include "VulkanCommandManager.h"
#include "VulkanContext.h"


VulkanCommandManager::VulkanCommandManager(const VulkanHandles& vulkanHandles, int MAX_FRAME_IN_FLIGHT):
	m_VulkanHandles(vulkanHandles)
{
	CreateCommandPool();
	CreateCommandBuffers(MAX_FRAME_IN_FLIGHT);
}

VulkanCommandManager::~VulkanCommandManager()
{
	// Giải phóng các command buffer chính.
	// LƯU Ý: Không cần gọi vkDestroyCommandBuffer vì chúng được giải phóng cùng với command pool.
	// vkFreeCommandBuffers chỉ đơn giản là trả chúng về pool.
	vkFreeCommandBuffers(m_VulkanHandles.device, m_Handles.commandPool, m_Handles.commandBuffers.size(), m_Handles.commandBuffers.data());
	
	// Hủy command pool, hành động này cũng sẽ giải phóng tất cả command buffer đã được cấp phát từ nó.
	vkDestroyCommandPool(m_VulkanHandles.device, m_Handles.commandPool, nullptr);
}

void VulkanCommandManager::CreateCommandPool()
{
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.queueFamilyIndex = m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	// Cờ này cho phép các command buffer có thể được reset riêng lẻ.
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(vkCreateCommandPool(m_VulkanHandles.device, &commandPoolInfo, nullptr, &m_Handles.commandPool), "LỖI: Tạo command pool thất bại!");
}

void VulkanCommandManager::CreateCommandBuffers(int MAX_FRAMES_IN_FLIGHT)
{
	m_Handles.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Buffer chính, có thể submit trực tiếp lên queue.
	allocInfo.commandPool = m_Handles.commandPool;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	VK_CHECK(vkAllocateCommandBuffers(m_VulkanHandles.device, &allocInfo, m_Handles.commandBuffers.data()), "LỖI: Cấp phát command buffer thất bại!");
}

VkCommandBuffer VulkanCommandManager::BeginSingleTimeCmdBuffer()
{
	// 1. Cấp phát một command buffer tạm thời.
	VkCommandBuffer cmdBuffer;
	VkCommandBufferAllocateInfo allocInfo{};

	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_Handles.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(m_VulkanHandles.device, &allocInfo, &cmdBuffer), "LỖI: Cấp phát command buffer dùng một lần thất bại!");

	// 2. Bắt đầu ghi command.
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// Cờ báo cho Vulkan biết rằng buffer này chỉ được submit một lần.
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = VK_NULL_HANDLE;

	vkBeginCommandBuffer(cmdBuffer, &beginInfo);

	return cmdBuffer;
}

void VulkanCommandManager::EndSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer)
{
	// 1. Kết thúc ghi command.
	vkEndCommandBuffer(cmdBuffer);

	// 2. Submit command buffer lên graphics queue.
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(m_VulkanHandles.graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
	
	// 3. Đợi cho queue thực thi xong.
	// LƯU Ý: vkDeviceWaitIdle là cách đơn giản nhất nhưng không hiệu quả nhất.
	// Một giải pháp tốt hơn là dùng fence để chỉ đợi command buffer này hoàn thành.
	vkDeviceWaitIdle(m_VulkanHandles.device);

	// 4. Giải phóng command buffer.
	vkFreeCommandBuffers(m_VulkanHandles.device, m_Handles.commandPool, 1, &cmdBuffer);
}
