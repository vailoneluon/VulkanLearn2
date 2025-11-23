#pragma once
#include "VulkanContext.h"
#include <vector>
#include <unordered_map>

// Struct chứa các handle và thông tin nội bộ của một VulkanDescriptor.
struct DescriptorHandles
{
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	uint32_t setIndex = 0;
	std::unordered_map<VkDescriptorType, uint32_t> descriptorCountByType;
};

// Struct chứa thông tin để cập nhật một binding dạng image/sampler.
struct ImageDescriptorUpdateInfo
{
	uint32_t binding;
	std::vector<VkDescriptorImageInfo> imageInfos;
	uint32_t firstArrayElement;
};


// Struct chứa thông tin để cập nhật một binding dạng buffer.
struct BufferDescriptorUpdateInfo
{
	uint32_t binding;
	//uint32_t bufferInfoCount;
	//VkDescriptorBufferInfo* bufferInfos;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	uint32_t firstArrayElement;
};

// Struct mô tả thông tin cho một binding trong một descriptor set layout.
struct BindingElementInfo
{
	uint32_t              binding;
	VkDescriptorType      descriptorType;
	uint32_t              descriptorCount;
	VkShaderStageFlags    stageFlags;
	const VkSampler*      pImmutableSamplers = nullptr;

	bool				  useBindless = false;

	uint32_t imageDescriptorUpdateInfoCount = 0;
	ImageDescriptorUpdateInfo* pImageDescriptorUpdates = nullptr;

	uint32_t bufferDescriptorUpdateInfoCount = 0;
	BufferDescriptorUpdateInfo* pBufferDescriptorUpdates = nullptr;
};

// Class đóng gói một Descriptor Set và Layout của nó.
// Nó định nghĩa cấu trúc của một set (các binding) và quản lý việc cấp phát, cập nhật set đó.
class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanHandles& vulkanHandles, const std::vector<BindingElementInfo>& bindingInfos, uint32_t setIndex);
	~VulkanDescriptor();

	// --- Getters ---
	const DescriptorHandles& getHandles() const { return m_Handles; }
	uint32_t getSetIndex() const { return m_Handles.setIndex; }

	// Cấp phát descriptor set từ một pool đã có.
	void AllocateDescriptorSet(const VkDescriptorPool& descriptorPool);

	// Cập nhật các binding trong set với thông tin image.
	void WriteImageSets(int updateCount, const ImageDescriptorUpdateInfo* pImageBindingInfo);
	// Cập nhật các binding trong set với thông tin buffer.
	void WriteBufferSets(int updateCount, const BufferDescriptorUpdateInfo* pBufferBindingInfo);

	void UpdateDescriptorSets();
private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	DescriptorHandles m_Handles;
	const std::vector<BindingElementInfo> m_BindingInfos;
	std::vector<ImageDescriptorUpdateInfo> m_ImageDescriptorUpdateInfos;
	std::vector<BufferDescriptorUpdateInfo> m_BufferDescriptorUpdateInfos;


	// --- Hàm helper private ---
	BindingElementInfo getBindingElementInfo(uint32_t binding);
	void CreateSetLayout(const std::vector<BindingElementInfo>& bindingInfos);
	void CountDescriptorsByType(const std::vector<BindingElementInfo>& bindingInfos);
};