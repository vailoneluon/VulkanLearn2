#pragma once
#include "VulkanContext.h"


// Struct chứa handle nội bộ của VulkanSampler.
struct SamplerHandles
{
	VkSampler sampler = VK_NULL_HANDLE;
};

// Class quản lý việc tạo và hủy một VkSampler.
// Sampler được dùng trong shader để đọc dữ liệu từ texture (image).
// Nó định nghĩa cách texture được lọc (filtering) và xử lý tọa độ (addressing).
class VulkanSampler
{
public:
	VulkanSampler(const VulkanHandles& vulkanHandles);
	~VulkanSampler();

	// Lấy ra VkSampler handle.
	const VkSampler& getSampler() const { return m_Handles.sampler; };

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	SamplerHandles m_Handles;
};