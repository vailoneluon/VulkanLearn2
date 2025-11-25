#include "pch.h"
#include "VulkanDescriptorManager.h"
#include "VulkanDescriptor.h"


VulkanDescriptorManager::VulkanDescriptorManager(const VulkanHandles& vulkanHandles) :
	m_VulkanHandles(vulkanHandles)
{
	
}

VulkanDescriptorManager::~VulkanDescriptorManager()
{
	// Hủy descriptor pool, hành động này sẽ tự động giải phóng tất cả các descriptor set
	// đã được cấp phát từ pool này.
	vkDestroyDescriptorPool(m_VulkanHandles.device, m_Handles.descriptorPool, nullptr);

	// LƯU Ý: Vòng lặp này giải phóng các đối tượng VulkanDescriptor, bao gồm cả các
	// DescriptorSetLayout của chúng. Điều này là đúng vì Manager sở hữu các descriptor này.
	for (const auto& descriptor : m_Handles.descriptors)
	{
		delete(descriptor);
	}
}

void VulkanDescriptorManager::AddDescriptors(const std::vector<VulkanDescriptor*>& descriptors)
{
	m_Handles.descriptors.insert(m_Handles.descriptors.end(), descriptors.begin(), descriptors.end());
}

void VulkanDescriptorManager::Finalize()
{
	// Thực hiện các bước khởi tạo.
	CreateDescriptorPool();
	AllocateDescriptorSets();
}

void VulkanDescriptorManager::CreateDescriptorPool()
{
	// 1. Đếm tổng số lượng descriptor cần thiết cho mỗi loại.
	CountTotalDescriptorsByType();

	// 2. Tạo danh sách các VkDescriptorPoolSize dựa trên kết quả đã đếm.
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(m_DescriptorCountByType.size());

	for (const auto&[descType, count] : m_DescriptorCountByType)
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = descType;
		poolSize.descriptorCount = count;

		poolSizes.push_back(poolSize);
	}

	// 3. Tạo Descriptor Pool.
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	// maxSets là tổng số lượng Descriptor Set có thể được cấp phát từ pool này.
	poolInfo.maxSets = static_cast<uint32_t>(m_Handles.descriptors.size());

	VK_CHECK(vkCreateDescriptorPool(m_VulkanHandles.device, &poolInfo, nullptr, &m_Handles.descriptorPool), "LỖI: Tạo descriptor pool thất bại!");
}

void VulkanDescriptorManager::AllocateDescriptorSets()
{
	// Yêu cầu mỗi đối tượng VulkanDescriptor tự cấp phát Descriptor Set của nó từ pool chung.
	for (const auto& descriptor : m_Handles.descriptors)
	{
		descriptor->AllocateDescriptorSet(m_Handles.descriptorPool);
	}
}

void VulkanDescriptorManager::CountTotalDescriptorsByType()
{
	// Duyệt qua từng VulkanDescriptor được quản lý.
	for (const auto& descriptor : m_Handles.descriptors)
	{
		// Lấy map đếm của descriptor đó và cộng dồn vào map tổng của manager.
		for (const auto& [descType, count] : descriptor->getHandles().descriptorCountByType)
		{
			m_DescriptorCountByType[descType] += count;
		}
	}
}
