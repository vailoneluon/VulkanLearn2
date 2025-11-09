#pragma once
#include "VulkanContext.h"
#include <vector>

// Struct chứa thông tin chi tiết về khả năng của swapchain trên một physical device.
struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	// Các lựa chọn tối ưu được tìm thấy.
	VkSurfaceFormatKHR chosenFormat;
	VkPresentModeKHR chosenPresentMode;

	// Các hàm helper để chọn ra format và present mode tốt nhất từ danh sách có sẵn.
	void ChooseSurfaceFormatKHR();
	void ChoosePresentModeKHR();
};

// Struct chứa các handle và thông tin của swapchain.
struct SwapchainHandles
{
	uint32_t swapchainImageCount = 0;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	SwapchainSupportDetails swapchainSupportDetails;
	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkImage> swapchainImages;
};

// Class quản lý việc tạo và hủy một VkSwapchainKHR và các VkImageView tương ứng.
// Swapchain là một chuỗi các image được dùng để trình chiếu lên màn hình.
class VulkanSwapchain
{
public:
	VulkanSwapchain(const VulkanHandles& vulkanHandles, GLFWwindow* window);
	~VulkanSwapchain();

	// Lấy các handle nội bộ.
	const SwapchainHandles& getHandles() const { return m_Handles; }

private:
	// --- Tham chiếu Vulkan và Window ---
	const VulkanHandles& m_VulkanHandles;
	GLFWwindow* m_Window;
	
	// --- Dữ liệu nội bộ ---
	SwapchainHandles m_Handles;

	// --- Hàm helper private ---
	// Truy vấn các thông tin chi tiết về khả năng hỗ trợ swapchain.
	void QuerySwapchainSupportDetails(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
	// Chọn kích thước (extent) cho swapchain.
	void ChooseSwapchainExtent();

	// Các hàm tạo tài nguyên chính.
	void CreateSwapchain();
	void CreateSwapchainImageViews();
};