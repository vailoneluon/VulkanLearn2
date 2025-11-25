#pragma once
#include "VulkanContext.h"
#include <vector>

// =================================================================================================
// Struct: SwapchainSupportDetails
// Mô tả: Chứa thông tin chi tiết về khả năng của swapchain trên một physical device.
//        Bao gồm capabilities (min/max image count, extent), danh sách các formats và present modes hỗ trợ.
// =================================================================================================
struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	// Các lựa chọn tối ưu được tìm thấy.
	VkSurfaceFormatKHR chosenFormat;
	VkPresentModeKHR chosenPresentMode;

	// Helper: Chọn ra format tốt nhất từ danh sách có sẵn (ưu tiên SRGB).
	void ChooseSurfaceFormatKHR();

	// Helper: Chọn ra present mode tốt nhất từ danh sách có sẵn (ưu tiên Mailbox).
	void ChoosePresentModeKHR();
};

// =================================================================================================
// Struct: SwapchainHandles
// Mô tả: Chứa các handle và thông tin của swapchain.
//        Bao gồm VkSwapchainKHR, các VkImage và VkImageView tương ứng.
// =================================================================================================
struct SwapchainHandles
{
	uint32_t swapchainImageCount = 0;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	SwapchainSupportDetails swapchainSupportDetails;
	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkImage> swapchainImages;
};

// =================================================================================================
// Class: VulkanSwapchain
// Mô tả: 
//      Quản lý việc tạo và hủy một VkSwapchainKHR và các VkImageView tương ứng.
//      Swapchain là một chuỗi các image được dùng để trình chiếu lên màn hình.
//      Class này chịu trách nhiệm chọn format, present mode và extent phù hợp cho swapchain.
// =================================================================================================
class VulkanSwapchain
{
public:
	// Constructor: Khởi tạo swapchain.
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung.
	//      window: Con trỏ đến cửa sổ GLFW.
	VulkanSwapchain(const VulkanHandles& vulkanHandles, GLFWwindow* window);

	// Destructor: Hủy swapchain và các image view.
	~VulkanSwapchain();

	// Getter: Lấy các handle nội bộ.
	const SwapchainHandles& getHandles() const { return m_Handles; }

private:
	// --- Tham chiếu Vulkan và Window ---
	const VulkanHandles& m_VulkanHandles;
	GLFWwindow* m_Window;
	
	// --- Dữ liệu nội bộ ---
	SwapchainHandles m_Handles;

	// --- Hàm helper private ---
	
	// Helper: Truy vấn các thông tin chi tiết về khả năng hỗ trợ swapchain.
	void QuerySwapchainSupportDetails(VkPhysicalDevice physDevice, VkSurfaceKHR surface);

	// Helper: Chọn kích thước (extent) cho swapchain dựa trên kích thước cửa sổ.
	void ChooseSwapchainExtent();

	// Helper: Tạo VkSwapchainKHR.
	void CreateSwapchain();

	// Helper: Tạo VkImageView cho từng image trong swapchain.
	void CreateSwapchainImageViews();
};