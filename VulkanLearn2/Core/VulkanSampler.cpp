#include "VulkanSampler.h"
#include "../Utils/ErrorHelper.h"

VulkanSampler::VulkanSampler(const VulkanHandles& vulkanHandles):
	vk(vulkanHandles)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(vk.physicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.compareEnable = VK_FALSE;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerInfo.minLod = 0.0f;
	samplerInfo.mipLodBias = 0;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	VK_CHECK(vkCreateSampler(vk.device, &samplerInfo, nullptr, &handles.sampler), "FAILED TO CREATE VULKAN SAMPLER");
}

VulkanSampler::~VulkanSampler()
{
	vkDestroySampler(vk.device, handles.sampler, nullptr);
}
