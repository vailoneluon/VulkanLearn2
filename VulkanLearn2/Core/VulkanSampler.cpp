#include "pch.h"
#include "VulkanSampler.h"

VulkanSampler::VulkanSampler(const VulkanHandles& vulkanHandles):
	m_VulkanHandles(vulkanHandles)
{
	// Lấy các thuộc tính của physical device để kiểm tra giới hạn, ví dụ như mức lọc bất đẳng hướng tối đa.
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_VulkanHandles.physicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Chế độ lọc (filtering) cho trường hợp texture được thu nhỏ (min) hoặc phóng to (mag).
	samplerInfo.minFilter = VK_FILTER_LINEAR; // Lọc tuyến tính (làm mượt ảnh).
	samplerInfo.magFilter = VK_FILTER_LINEAR;

	// Chế độ xử lý tọa độ (address mode) khi tọa độ texture ra ngoài khoảng [0, 1].
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // Lặp lại texture.
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// Lọc bất đẳng hướng (Anisotropic Filtering), giúp cải thiện chất lượng texture khi nhìn ở góc xiên.
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // Sử dụng mức tối đa mà GPU hỗ trợ.

	// So sánh texture với một giá trị tham chiếu (thường dùng cho shadow map), ở đây không dùng.
	samplerInfo.compareEnable = VK_FALSE;

	// Chế độ xử lý mipmap.
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // Lọc tuyến tính giữa các cấp mipmap.
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE; // Sử dụng tất cả các cấp mipmap có sẵn.

	// Tọa độ không chuẩn hóa (false nghĩa là tọa độ trong khoảng [0, 1]).
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	VK_CHECK(vkCreateSampler(m_VulkanHandles.device, &samplerInfo, nullptr, &m_Handles.sampler), "LỖI: Tạo sampler thất bại!");
}

VulkanSampler::~VulkanSampler()
{
	vkDestroySampler(m_VulkanHandles.device, m_Handles.sampler, nullptr);
}