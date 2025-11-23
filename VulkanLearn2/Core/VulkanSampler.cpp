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

	// --- Cấu hình Shadow Sampler ---
	// Shadow sampler cần bộ lọc tuyến tính (VK_FILTER_LINEAR) và chế độ so sánh depth.
	// Nó cũng nên có chế độ địa chỉ là CLAMP_TO_BORDER với màu border là trắng (1.0)
	// để các pixel nằm ngoài vùng shadow map không tạo bóng.
	VkSamplerCreateInfo shadowSamplerInfo{};
	shadowSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	shadowSamplerInfo.minFilter = VK_FILTER_LINEAR;
	shadowSamplerInfo.magFilter = VK_FILTER_LINEAR;
	shadowSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // Clamp để xử lý các vùng ngoài shadow map
	shadowSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowSamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // Pixel ngoài sẽ là trắng (không đổ bóng)
	shadowSamplerInfo.anisotropyEnable = VK_FALSE; // Không cần anisotropic cho shadow map
	shadowSamplerInfo.compareEnable = VK_TRUE;     // Bật chế độ so sánh depth
	shadowSamplerInfo.compareOp = VK_COMPARE_OP_LESS; // So sánh depth: nếu < giá trị sampler thì không đổ bóng
	shadowSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	shadowSamplerInfo.mipLodBias = 0.0f;
	shadowSamplerInfo.minLod = 0.0f;
	shadowSamplerInfo.maxLod = 1.0f; // Chỉ dùng mip level 0 (hoặc tối thiểu) cho shadow map
	shadowSamplerInfo.unnormalizedCoordinates = VK_FALSE;

	VK_CHECK(vkCreateSampler(m_VulkanHandles.device, &shadowSamplerInfo, nullptr, &m_Handles.shadowSampler), "LỖI: Tạo shadow sampler thất bại!");
}

VulkanSampler::~VulkanSampler()
{
	vkDestroySampler(m_VulkanHandles.device, m_Handles.shadowSampler, nullptr);
	vkDestroySampler(m_VulkanHandles.device, m_Handles.sampler, nullptr);
}