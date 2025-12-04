#pragma once
#include "VulkanContext.h"


// =================================================================================================
// Struct: SamplerHandles
// Mô tả: Chứa handle nội bộ của VulkanSampler.
//        Bao gồm VkSampler chính và VkSampler cho shadow mapping.
// =================================================================================================
struct SamplerHandles
{
	VkSampler sampler = VK_NULL_HANDLE;
	VkSampler shadowSampler = VK_NULL_HANDLE;
	VkSampler postProcessSampler = VK_NULL_HANDLE;
};

// =================================================================================================
// Class: VulkanSampler
// Mô tả: 
//      Quản lý việc tạo và hủy một VkSampler.
//      Sampler được dùng trong shader để đọc dữ liệu từ texture (image).
//      Nó định nghĩa cách texture được lọc (filtering) và xử lý tọa độ (addressing).
// =================================================================================================
class VulkanSampler
{
public:
	// Constructor: Khởi tạo sampler và shadow sampler.
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung.
	VulkanSampler(const VulkanHandles& vulkanHandles);
	~VulkanSampler();

	// Getter: Lấy ra VkSampler handle.
	const VkSampler& getSampler() const { return m_Handles.sampler; };

	// Getter: Lấy ra Shadow Sampler
	const VkSampler& getShadowSampler() const { return m_Handles.shadowSampler; }

	// Getter: Lấy ra PostProcess Sampler
	const VkSampler& getPostProcessSampler() const { return m_Handles.postProcessSampler; }

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	SamplerHandles m_Handles;
};