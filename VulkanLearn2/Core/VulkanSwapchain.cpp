#include "pch.h"
#include "VulkanSwapchain.h"


VulkanSwapchain::VulkanSwapchain(const VulkanHandles& vulkanHandles, GLFWwindow* window):
	vk(vulkanHandles), window(window)
{
	querrySwapchainSupportDetails(vk.physicalDevice, vk.surface);

	CreateSwapchain();
	CreateSwapchainImageview();
}

VulkanSwapchain::~VulkanSwapchain()
{
	vkDestroySwapchainKHR(vk.device, handles.swapchain, nullptr);
	
	for (int i = 0; i < handles.swapchainImageCount; i++)
	{
		vkDestroyImageView(vk.device, handles.swapchainImageViews[i], nullptr);
	}
}

void VulkanSwapchain::querrySwapchainSupportDetails(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &handles.swapchainSuportDetails.capabilities), "FAILED TO GET SURFACE CAPABILITIES");
	
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
	handles.swapchainSuportDetails.formats.resize(formatCount);
	if (formatCount == 0)
	{
		throw std::runtime_error("DONT HAVE SUPPORT SURFACE FORMAT");
	}
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, handles.swapchainSuportDetails.formats.data());

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount == 0)
	{
		throw std::runtime_error("DONT HAVE SUPPORT PRESENT MODE");
	}
	handles.swapchainSuportDetails.presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, handles.swapchainSuportDetails.presentModes.data());

	handles.swapchainSuportDetails.ChooseSurfaceFormatKHR();
	handles.swapchainSuportDetails.ChoosePresentModeKHR();

	chooseSwapchainExtent();
}

void VulkanSwapchain::chooseSwapchainExtent()
{
	VkExtent2D extent;
	extent = handles.swapchainSuportDetails.capabilities.currentExtent;
	if (extent.width != UINT32_MAX)
	{
		handles.swapChainExtent = extent;
		return;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		extent = { static_cast<uint32_t> (width), static_cast<uint32_t>(height) };
		extent.width = std::clamp(extent.width, handles.swapchainSuportDetails.capabilities.minImageExtent.width, handles.swapchainSuportDetails.capabilities.maxImageExtent.width);
		extent.height = std::clamp(extent.height, handles.swapchainSuportDetails.capabilities.minImageExtent.height, handles.swapchainSuportDetails.capabilities.maxImageExtent.height);
		handles.swapChainExtent = extent;
		return;
	}
}

void SwapchainSupportDetails::ChooseSurfaceFormatKHR()
{
	for (const VkSurfaceFormatKHR& format : formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			chosenFormat = format;
			break;
		}
	}
}

void SwapchainSupportDetails::ChoosePresentModeKHR()
{
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			/*chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			return;*/
		}
	}
	chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanSwapchain::CreateSwapchain()
{
	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = vk.surface;

	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.minImageCount = handles.swapchainSuportDetails.capabilities.minImageCount + 1;

	swapchainInfo.imageColorSpace = handles.swapchainSuportDetails.chosenFormat.colorSpace;
	swapchainInfo.imageFormat = handles.swapchainSuportDetails.chosenFormat.format;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageExtent = handles.swapChainExtent;

	swapchainInfo.presentMode = handles.swapchainSuportDetails.chosenPresentMode;

	if (vk.queueFamilyIndices.GraphicQueueIndex == vk.queueFamilyIndices.PresentQueueIndex)
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.queueFamilyIndexCount = 0;
		swapchainInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		uint32_t indices[] = { vk.queueFamilyIndices.GraphicQueueIndex, vk.queueFamilyIndices.PresentQueueIndex };
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = indices;
	}

	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

	swapchainInfo.preTransform = handles.swapchainSuportDetails.capabilities.currentTransform;

	VK_CHECK(vkCreateSwapchainKHR(vk.device, &swapchainInfo, nullptr, &handles.swapchain), "FAILED TO CREATE SWAPCHAIN");
}

void VulkanSwapchain::CreateSwapchainImageview()
{
	std::vector<VkImage> swapchainImages;

	vkGetSwapchainImagesKHR(vk.device, handles.swapchain, &handles.swapchainImageCount, nullptr);
	swapchainImages.resize(handles.swapchainImageCount);
	handles.swapchainImageViews.resize(handles.swapchainImageCount);
	vkGetSwapchainImagesKHR(vk.device, handles.swapchain, &handles.swapchainImageCount, swapchainImages.data());

	for (int i = 0; i < handles.swapchainImageCount; i++)
	{
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = swapchainImages[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = handles.swapchainSuportDetails.chosenFormat.format;

		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;

		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;

		VK_CHECK(vkCreateImageView(vk.device, &imageViewInfo, nullptr, &handles.swapchainImageViews[i]), "FAILED TO CREATE SWAPCHAIN IMAGE VIEW");
	}
}
