#include "pch.h"
#include "VulkanDescriptor.h"


VulkanDescriptor::VulkanDescriptor(const VulkanHandles& vulkanHandles, const std::vector<BindingElementInfo>& bindingInfos, uint32_t setIndex):
	m_VulkanHandles(vulkanHandles), 
	m_BindingInfos(bindingInfos)
{
	m_Handles.setIndex = setIndex;

	for (const auto& bindingInfo : bindingInfos)
	{
		for (int i = 0; i< bindingInfo.bufferDescriptorUpdateInfoCount; i++)
		{
			m_BufferDescriptorUpdateInfos.push_back(bindingInfo.pBufferDescriptorUpdates[i]);
		}
		for (int i = 0; i < bindingInfo.imageDescriptorUpdateInfoCount; i++)
		{
			m_ImageDescriptorUpdateInfos.push_back(bindingInfo.pImageDescriptorUpdates[i]);
		}
	}
	
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

	bool hasBindless = false;
	std::vector<VkDescriptorBindingFlags> bindingFlags;

	for (const auto& bindingInfo : bindingInfos)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = bindingInfo.binding;
		layoutBinding.pImmutableSamplers = bindingInfo.pImmutableSamplers;
		layoutBinding.descriptorCount = bindingInfo.descriptorCount;
		layoutBinding.descriptorType = bindingInfo.descriptorType;
		layoutBinding.stageFlags = bindingInfo.stageFlags;

		layoutBindings.push_back(layoutBinding);

		if (bindingInfo.useBindless)
		{
			hasBindless = true;
			bindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
		}
		else
		{
			bindingFlags.push_back(0);
		}
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	VkDescriptorSetLayoutBindingFlagsCreateInfo flagInfo{};
	if (hasBindless)
	{
		flagInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		flagInfo.bindingCount = bindingFlags.size();
		flagInfo.pBindingFlags = bindingFlags.data();
		layoutInfo.pNext = &flagInfo;
	}

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

	UpdateDescriptorSets();
}

void VulkanDescriptor::WriteImageSets(int updateCount, const ImageDescriptorUpdateInfo* pImageBindingInfo)
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
		descriptorSetWrite.dstArrayElement = pImageBindingInfo[i].firstArrayElement;
		descriptorSetWrite.descriptorCount = pImageBindingInfo[i].imageInfos.size();
		descriptorSetWrite.pImageInfo = pImageBindingInfo[i].imageInfos.data();

		descriptorSetWrites.push_back(descriptorSetWrite);
	}

	// Thực hiện cập nhật tất cả các binding trong một lần gọi.
	vkUpdateDescriptorSets(m_VulkanHandles.device, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, nullptr);
}

void VulkanDescriptor::WriteBufferSets(int updateCount, const BufferDescriptorUpdateInfo* pBufferBindingInfo)
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
		descriptorSetWrite.dstArrayElement = pBufferBindingInfo[i].firstArrayElement;
		descriptorSetWrite.descriptorCount = pBufferBindingInfo[i].bufferInfos.size();
		descriptorSetWrite.pBufferInfo =  pBufferBindingInfo[i].bufferInfos.data();

		descriptorSetWrites.push_back(descriptorSetWrite);
	}

	vkUpdateDescriptorSets(m_VulkanHandles.device, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, nullptr);
}

void VulkanDescriptor::UpdateDescriptorSets()
{
	WriteBufferSets(m_BufferDescriptorUpdateInfos.size(), m_BufferDescriptorUpdateInfos.data());
	WriteImageSets(m_ImageDescriptorUpdateInfos.size(), m_ImageDescriptorUpdateInfos.data());
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