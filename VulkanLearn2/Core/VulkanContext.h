#pragma once

#include <vector>
#include <string>
#include <set>

// =================================================================================================
// Struct: QueueFamilyIndices
// Mô tả: Lưu trữ chỉ số (index) của các queue family cần thiết cho ứng dụng.
// =================================================================================================
struct QueueFamilyIndices
{
	uint32_t GraphicQueueIndex; // Index cho graphics queue family
	uint32_t PresentQueueIndex; // Index cho presentation queue family
};

// =================================================================================================
// Struct: VulkanHandles
// Mô tả: Đóng gói các handle cốt lõi của Vulkan được sử dụng trong toàn bộ ứng dụng.
// =================================================================================================
struct VulkanHandles
{
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VmaAllocator allocator = VK_NULL_HANDLE;

	QueueFamilyIndices queueFamilyIndices{};
	VkQueue graphicQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
};

// =================================================================================================
// Class: VulkanContext
// Mô tả: 
//      Chịu trách nhiệm khởi tạo và quản lý các thành phần cốt lõi của Vulkan:
//      - Instance, Device, Physical Device, Surface, Queues.
//      - Đóng gói tất cả các bước thiết lập ban đầu phức tạp.
// =================================================================================================
class VulkanContext
{
public:
	// Constructor: Khởi tạo Vulkan context.
	// Tham số:
	//      window: Con trỏ đến cửa sổ GLFW.
	//      extensions: Danh sách các instance extension cần thiết.
	VulkanContext(GLFWwindow* window, std::vector<const char*> extensions);
	
	// Destructor: Dọn dẹp tài nguyên Vulkan.
	~VulkanContext();

	// Getter: Trả về struct chứa các handle Vulkan cốt lõi.
	const VulkanHandles& getVulkanHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	VulkanHandles m_Handles;

	// --- Cấu hình ---
	const char* m_ValidationLayers[1] = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> m_InstanceExtensionsRequired = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
	std::vector<const char*> m_DeviceExtensionsRequired = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

	// --- Các hàm Helper khởi tạo ---
	void CreateInstance(const std::vector<const char*>& extensions);
	void CreateSurface(GLFWwindow* window);
	void ChoosePhysicalDevice();
	void CreateLogicalDevice();
	void CreateVMAAllocator();

	// --- Debug Messenger ---
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateDebugMessenger();

	// Helper: Kiểm tra xem physical device có đáp ứng yêu cầu không.
	bool isPhysicalDeviceSuitable(VkPhysicalDevice physDevice);
};