#pragma once
#include "VulkanContext.h"
#include <vector>
#include <unordered_map>

// Forward declaration
class VulkanDescriptor;

// =================================================================================================
// Struct: DescriptorManagerHandles
// Mô tả: Chứa các handle nội bộ của VulkanDescriptorManager.
//        Bao gồm VkDescriptorPool và danh sách các VulkanDescriptor được quản lý.
// =================================================================================================
struct DescriptorManagerHandles
{
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	// Danh sách tất cả các VulkanDescriptor được quản lý.
	std::vector<VulkanDescriptor*> descriptors;
};

// =================================================================================================
// Class: VulkanDescriptorManager
// Mô tả: 
//      Quản lý việc tạo Descriptor Pool và cấp phát các Descriptor Set.
//      Nó tổng hợp yêu cầu từ nhiều VulkanDescriptor khác nhau để tạo ra một pool đủ lớn,
//      sau đó điều phối việc cấp phát set cho từng descriptor.
// =================================================================================================
class VulkanDescriptorManager
{
public:
	// Constructor: Khởi tạo manager.
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung.
	VulkanDescriptorManager(const VulkanHandles& vulkanHandles);
	~VulkanDescriptorManager();

	// Thêm các descriptor vào danh sách quản lý.
	void AddDescriptors(const std::vector<VulkanDescriptor*>& descriptors);

	// Hoàn tất việc thiết lập: Tạo pool và cấp phát set.
	void Finalize();

	// Getter: Lấy các handle nội bộ.
	const DescriptorManagerHandles& getHandles() const { return m_Handles; }

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	DescriptorManagerHandles m_Handles;
	
	// Map lưu tổng số lượng descriptor cần thiết cho mỗi loại.
	std::unordered_map<VkDescriptorType, uint32_t> m_DescriptorCountByType;

	// --- Hàm helper private ---
	
	// Helper: Tạo Descriptor Pool.
	void CreateDescriptorPool();
	
	// Helper: Cấp phát Descriptor Set cho từng descriptor.
	void AllocateDescriptorSets();

	// Helper: Đếm tổng số descriptor mỗi loại từ tất cả các VulkanDescriptor được quản lý.
	void CountTotalDescriptorsByType();
};