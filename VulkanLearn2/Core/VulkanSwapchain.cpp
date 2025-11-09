#include "pch.h"
#include "VulkanSwapchain.h"

VulkanSwapchain::VulkanSwapchain(const VulkanHandles& vulkanHandles, GLFWwindow* window):
	m_VulkanHandles(vulkanHandles), 
	m_Window(window)
{
	// 1. Truy vấn các khả năng của physical device để xác định các thông số phù hợp cho swapchain.
	QuerySwapchainSupportDetails(m_VulkanHandles.physicalDevice, m_VulkanHandles.surface);

	// 2. Tạo swapchain.
	CreateSwapchain();

	// 3. Tạo các image view cho từng image trong swapchain.
	CreateSwapchainImageViews();
}

VulkanSwapchain::~VulkanSwapchain()
{
	// Hủy các image view trước.
	for (size_t i = 0; i < m_Handles.swapchainImageCount; i++)
	{
		vkDestroyImageView(m_VulkanHandles.device, m_Handles.swapchainImageViews[i], nullptr);
	}

	// Sau đó hủy swapchain.
	vkDestroySwapchainKHR(m_VulkanHandles.device, m_Handles.swapchain, nullptr);
}

void VulkanSwapchain::QuerySwapchainSupportDetails(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	// Lấy các capabilities của surface (min/max image count, extent, v.v.).
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &m_Handles.swapchainSupportDetails.capabilities), "LỖI: Lấy surface capabilities thất bại!");
	
	// Lấy danh sách các format được hỗ trợ.
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		m_Handles.swapchainSupportDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, m_Handles.swapchainSupportDetails.formats.data());
	}

	// Lấy danh sách các present mode được hỗ trợ.
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		m_Handles.swapchainSupportDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, m_Handles.swapchainSupportDetails.presentModes.data());
	}

	// Chọn ra format và present mode tốt nhất.
	m_Handles.swapchainSupportDetails.ChooseSurfaceFormatKHR();
	m_Handles.swapchainSupportDetails.ChoosePresentModeKHR();

	// Chọn kích thước cho swapchain.
	ChooseSwapchainExtent();
}

void VulkanSwapchain::ChooseSwapchainExtent()
{
	// Nếu currentExtent không phải là giá trị đặc biệt, ta có thể dùng luôn nó.
	if (m_Handles.swapchainSupportDetails.capabilities.currentExtent.width != UINT32_MAX)
	{
		m_Handles.swapChainExtent = m_Handles.swapchainSupportDetails.capabilities.currentExtent;
	}
	else // Nếu không, ta cần tự xác định kích thước dựa trên cửa sổ.
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		// Kẹp giá trị trong khoảng min/max cho phép.
		actualExtent.width = std::clamp(actualExtent.width, m_Handles.swapchainSupportDetails.capabilities.minImageExtent.width, m_Handles.swapchainSupportDetails.capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, m_Handles.swapchainSupportDetails.capabilities.minImageExtent.height, m_Handles.swapchainSupportDetails.capabilities.maxImageExtent.height);
		
		m_Handles.swapChainExtent = actualExtent;
	}
}

void SwapchainSupportDetails::ChooseSurfaceFormatKHR()
{
	// Ưu tiên tìm format B8G8R8A8_SRGB với không gian màu SRGB.
	for (const VkSurfaceFormatKHR& format : formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			chosenFormat = format;
			return;
		}
	}
	// Nếu không tìm thấy, dùng cái đầu tiên trong danh sách.
	chosenFormat = formats[0];
}

void SwapchainSupportDetails::ChoosePresentModeKHR()
{
	// Ưu tiên tìm chế độ MAILBOX (triple buffering) để giảm thiểu tearing và latency.
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			/*chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			return;*/
		}
	}
	// Nếu không có, dùng chế độ FIFO (vsync), được đảm bảo luôn có.
	chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanSwapchain::CreateSwapchain()
{
	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = m_VulkanHandles.surface;

	// Số lượng image trong swapchain. Nên nhiều hơn minImageCount để có không gian cho buffering.
	uint32_t imageCount = m_Handles.swapchainSupportDetails.capabilities.minImageCount + 1;
	if (m_Handles.swapchainSupportDetails.capabilities.maxImageCount > 0 && imageCount > m_Handles.swapchainSupportDetails.capabilities.maxImageCount)
	{
		imageCount = m_Handles.swapchainSupportDetails.capabilities.maxImageCount;
	}
	swapchainInfo.minImageCount = imageCount;

	swapchainInfo.imageFormat = m_Handles.swapchainSupportDetails.chosenFormat.format;
	swapchainInfo.imageColorSpace = m_Handles.swapchainSupportDetails.chosenFormat.colorSpace;
	swapchainInfo.imageExtent = m_Handles.swapChainExtent;
	swapchainInfo.imageArrayLayers = 1; // Luôn là 1 trừ khi làm 3D stereoscopic.
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Dùng image làm color attachment.

	// Xử lý trường hợp graphic queue và present queue khác nhau.
	if (m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex != m_VulkanHandles.queueFamilyIndices.PresentQueueIndex)
	{
		uint32_t indices[] = { m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex, m_VulkanHandles.queueFamilyIndices.PresentQueueIndex };
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image được chia sẻ giữa 2 queue.
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = indices;
	}
	else
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Chỉ một queue sử dụng, hiệu năng cao hơn.
	}

	swapchainInfo.preTransform = m_Handles.swapchainSupportDetails.capabilities.currentTransform; // Không áp dụng transform nào.
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Không trộn alpha với cửa sổ của HĐH.
	swapchainInfo.presentMode = m_Handles.swapchainSupportDetails.chosenPresentMode;
	swapchainInfo.clipped = VK_TRUE; // Bỏ qua các pixel bị che khuất.
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE; // Dùng khi resize cửa sổ, ở đây không xử lý.

	VK_CHECK(vkCreateSwapchainKHR(m_VulkanHandles.device, &swapchainInfo, nullptr, &m_Handles.swapchain), "LỖI: Tạo swapchain thất bại!");
}

void VulkanSwapchain::CreateSwapchainImageViews()
{
	// Lấy các handle của VkImage từ swapchain vừa tạo.
	vkGetSwapchainImagesKHR(m_VulkanHandles.device, m_Handles.swapchain, &m_Handles.swapchainImageCount, nullptr);
	m_Handles.swapchainImages.resize(m_Handles.swapchainImageCount);
	m_Handles.swapchainImageViews.resize(m_Handles.swapchainImageCount);
	vkGetSwapchainImagesKHR(m_VulkanHandles.device, m_Handles.swapchain, &m_Handles.swapchainImageCount, m_Handles.swapchainImages.data());

	// Tạo một VkImageView cho mỗi VkImage.
	for (size_t i = 0; i < m_Handles.swapchainImageCount; i++)
	{
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = m_Handles.swapchainImages[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = m_Handles.swapchainSupportDetails.chosenFormat.format;

		// components cho phép "swizzle" các kênh màu, ở đây để mặc định.
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// subresourceRange mô tả mục đích và phần nào của image sẽ được truy cập.
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(m_VulkanHandles.device, &imageViewInfo, nullptr, &m_Handles.swapchainImageViews[i]), "LỖI: Tạo swapchain image view thất bại!");
	}
}