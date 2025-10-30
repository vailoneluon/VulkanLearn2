#pragma once
#include "VulkanContext.h"
#include <vector>
#include <unordered_map>

// Forward declaration
class VulkanDescriptor;

// Struct chứa các handle nội bộ của VulkanDescriptorManager.
struct DescriptorManagerHandles
{
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	// Danh sách tất cả các VulkanDescriptor được quản lý.
	std::vector<VulkanDescriptor*> descriptors;
};

// Class quản lý việc tạo Descriptor Pool và cấp phát các Descriptor Set.
// Nó tổng hợp yêu cầu từ nhiều VulkanDescriptor khác nhau để tạo ra một pool đủ lớn,
// sau đó điều phối việc cấp phát set cho từng descriptor.
class VulkanDescriptorManager
{
public:
	VulkanDescriptorManager(const VulkanHandles& vulkanHandles, std::vector<VulkanDescriptor*>& vulkanDescriptors);
	~VulkanDescriptorManager();

	// Lấy các handle nội bộ.
	const DescriptorManagerHandles& getHandles() const { return m_Handles; }

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	DescriptorManagerHandles m_Handles;
	// Map lưu tổng số lượng descriptor cần thiết cho mỗi loại.
	std::unordered_map<VkDescriptorType, uint32_t> m_DescriptorCountByType;

	// --- Hàm helper private ---
	void CreateDescriptorPool();
	void AllocateDescriptorSets();

	// Đếm tổng số descriptor mỗi loại từ tất cả các VulkanDescriptor được quản lý.
	void CountTotalDescriptorsByType();
};