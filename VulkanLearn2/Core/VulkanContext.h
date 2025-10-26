#pragma once

struct QueueFamilyIndices
{
	uint32_t GraphicQueueIndex;
	uint32_t PresentQueueIndex;
};

struct VulkanHandles
{
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;

	QueueFamilyIndices queueFamilyIndices{};
	VkQueue graphicQueue;
	VkQueue presentQueue;
};

class VulkanContext
{
public:
	VulkanContext(GLFWwindow* window, std::vector<const char*> extensions);
	~VulkanContext();

	const VulkanHandles& getVulkanHandles() const { return handles; }

private:
	
	VulkanHandles handles;

	const char* validationLayers[1] = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> instanceExtensionsRequired = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
	std::vector<const char*> physicalRequireDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


	void CreateInstance(std::vector<const char*> extensions);
	void CreateSurface(GLFWwindow* window);
	void ChoosePhysicalDevice();
	void CreateLogicalDevice();

	bool isPhysicalDeviceSuitable(VkPhysicalDevice physDevice);
};