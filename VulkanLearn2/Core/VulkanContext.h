#pragma once

#include <vector>
#include <string>
#include <set>

// Struct lưu trữ chỉ số (index) của các queue family cần thiết.
struct QueueFamilyIndices
{
	uint32_t GraphicQueueIndex;
	uint32_t PresentQueueIndex;
};


struct VulkanHandles
{
	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	QueueFamilyIndices queueFamilyIndices{};
	VkQueue graphicQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
};

// Class VulkanContext chịu trách nhiệm khởi tạo các thành phần cốt lõi của Vulkan:
// - Instance, Device, Physical Device, Surface, Queues.
// Nó đóng gói tất cả các bước thiết lập ban đầu phức tạp.
class VulkanContext
{
public:
	// Constructor, nhận vào cửa sổ GLFW và các instance extension cần thiết từ cửa sổ.
	VulkanContext(GLFWwindow* window, std::vector<const char*> extensions);
	~VulkanContext();

	// Lấy ra struct chứa các handle Vulkan chính.
	const VulkanHandles& getVulkanHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	VulkanHandles m_Handles;

	// --- Cấu hình khởi tạo ---
	const char* m_ValidationLayers[1] = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> m_InstanceExtensionsRequired = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
	std::vector<const char*> m_DeviceExtensionsRequired = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


	// --- Hàm helper private cho việc khởi tạo ---
	void CreateInstance(const std::vector<const char*>& extensions);
	void CreateSurface(GLFWwindow* window);
	void ChoosePhysicalDevice();
	void CreateLogicalDevice();

	// Kiểm tra xem một physical device có phù hợp với các yêu cầu không.
	bool isPhysicalDeviceSuitable(VkPhysicalDevice physDevice);
};