#include "pch.h"
#include "VulkanDescriptor.h"


VulkanDescriptor::VulkanDescriptor(const VulkanHandles& vulkanHandles, const std::vector<BindingElementInfo>& bindingInfos, uint32_t setIndex):
	m_VulkanHandles(vulkanHandles), 
	m_BindingInfos(bindingInfos)
{
	m_Handles.setIndex = setIndex;

	CreateSetLayout(bindingInfos);
	CountDescriptorsByType(bindingInfos);
}

VulkanDescriptor::~VulkanDescriptor()
{
	// LƯU Ý: Descriptor set được cấp phát từ Descriptor Pool và sẽ được giải phóng
	// cùng lúc khi pool bị hủy, nên không cần giải phóng riêng lẻ ở đây.
	// Tuy nhiên, layout là tài nguyên độc lập và cần được hủy.
	vkDestroyDescriptorSetLayout(m_VulkanHandles.device, m_Handles.descriptorSetLayout, nullptr);
}

BindingElementInfo VulkanDescriptor::getBindingElementInfo(uint32_t binding)
{
	for (const auto& bindingInfo : m_BindingInfos)
	{
		if (bindingInfo.binding == binding)
		{
			return bindingInfo;
		}
	}

	// Nếu không tìm thấy, đó là lỗi nghiêm trọng.
	throw std::runtime_error("LỖI: Không tìm thấy thông tin binding với chỉ số đã cho!");
}

void VulkanDescriptor::CreateSetLayout(const std::vector<BindingElementInfo>& bindingInfos)
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	layoutBindings.reserve(bindingInfos.size());

	for (const auto& bindingInfo : bindingInfos)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = bindingInfo.binding;
		layoutBinding.pImmutableSamplers = bindingInfo.pImmutableSamplers;
		layoutBinding.descriptorCount = bindingInfo.descriptorCount;
		layoutBinding.descriptorType = bindingInfo.descriptorType;
		layoutBinding.stageFlags = bindingInfo.stageFlags;

		layoutBindings.push_back(layoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	VK_CHECK(vkCreateDescriptorSetLayout(m_VulkanHandles.device, &layoutInfo, nullptr, &m_Handles.descriptorSetLayout), "LỖI: Tạo descriptor set layout thất bại!");
}

void VulkanDescriptor::AllocateDescriptorSet(const VkDescriptorPool& descriptorPool)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_Handles.descriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(m_VulkanHandles.device, &allocInfo, &m_Handles.descriptorSet), "LỖI: Cấp phát descriptor set thất bại!");
}

void VulkanDescriptor::UpdateImageBinding(int updateCount, const ImageDescriptorUpdateInfo* pImageBindingInfo)
{
	std::vector<VkWriteDescriptorSet> descriptorSetWrites;
	descriptorSetWrites.reserve(updateCount);

	for (int i = 0; i < updateCount; i++)
	{
		VkWriteDescriptorSet descriptorSetWrite{};

		// Lấy thông tin binding gốc để đảm bảo type và count là chính xác.
		BindingElementInfo bindingInfo = getBindingElementInfo(pImageBindingInfo[i].binding);

		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstSet = m_Handles.descriptorSet;
		descriptorSetWrite.dstBinding = bindingInfo.binding;
		descriptorSetWrite.descriptorType = bindingInfo.descriptorType;
		descriptorSetWrite.descriptorCount = pImageBindingInfo[i].imageInfoCount;
		descriptorSetWrite.dstArrayElement = pImageBindingInfo[i].firstArrayElement;
		descriptorSetWrite.pImageInfo = pImageBindingInfo[i].imageInfos;

		descriptorSetWrites.push_back(descriptorSetWrite);
	}

	// Thực hiện cập nhật tất cả các binding trong một lần gọi.
	vkUpdateDescriptorSets(m_VulkanHandles.device, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, nullptr);
}

void VulkanDescriptor::UpdateBufferBinding(int updateCount, const BufferDescriptorUpdateInfo* pBufferBindingInfo)
{
	std::vector<VkWriteDescriptorSet> descriptorSetWrites;
	descriptorSetWrites.reserve(updateCount);

	for (int i = 0; i < updateCount; i++)
	{
		VkWriteDescriptorSet descriptorSetWrite{};

		BindingElementInfo bindingInfo = getBindingElementInfo(pBufferBindingInfo[i].binding);

		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstSet = m_Handles.descriptorSet;
		descriptorSetWrite.dstBinding = bindingInfo.binding;
		descriptorSetWrite.descriptorType = bindingInfo.descriptorType;
		descriptorSetWrite.descriptorCount = bindingInfo.descriptorCount;
		descriptorSetWrite.dstArrayElement = 0;
		descriptorSetWrite.pBufferInfo = &pBufferBindingInfo[i].bufferInfo;

		descriptorSetWrites.push_back(descriptorSetWrite);
	}

	vkUpdateDescriptorSets(m_VulkanHandles.device, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, nullptr);
}

void VulkanDescriptor::CountDescriptorsByType(const std::vector<BindingElementInfo>& bindingInfos)
{
	// Đếm tổng số lượng descriptor mỗi loại cần thiết cho set này.
	// Thông tin này sẽ được VulkanDescriptorManager sử dụng để tạo ra một Descriptor Pool đủ lớn.
	for (const auto& bindingInfo : bindingInfos)
	{
		m_Handles.descriptorCountByType[bindingInfo.descriptorType] += bindingInfo.descriptorCount;
	}
}