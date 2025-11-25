#include "pch.h"
#include "VulkanContext.h"


VulkanContext::VulkanContext(GLFWwindow* window, std::vector<const char*> instanceExtensions)
{
	// Tuần tự thực hiện các bước khởi tạo Vulkan.
	CreateInstance(instanceExtensions);
	CreateDebugMessenger();
	CreateSurface(window);
	ChoosePhysicalDevice();
	CreateLogicalDevice();
	CreateVMAAllocator();
}

VulkanContext::~VulkanContext()
{
	// Hủy các tài nguyên theo thứ tự ngược lại với lúc tạo.
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handles.instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(m_Handles.instance, m_Handles.debugMessenger, nullptr);
	}

	vmaDestroyAllocator(m_Handles.allocator);
	vkDestroySurfaceKHR(m_Handles.instance, m_Handles.surface, nullptr);
	vkDestroyDevice(m_Handles.device, nullptr);
	vkDestroyInstance(m_Handles.instance, nullptr);
}

void VulkanContext::CreateInstance(const std::vector<const char*>& extensions)
{
	// Gộp các extension yêu cầu bởi GLFW vào danh sách extension chung.
	m_InstanceExtensionsRequired.insert(m_InstanceExtensionsRequired.end(), extensions.begin(), extensions.end());

	// Thông tin về ứng dụng (tùy chọn nhưng nên có).
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName = "ZOLCOL ENGINE";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	// Thông tin để tạo Vulkan instance.
	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(m_InstanceExtensionsRequired.size());
	instanceInfo.ppEnabledExtensionNames = m_InstanceExtensionsRequired.data();
	instanceInfo.enabledLayerCount = 1;
	instanceInfo.ppEnabledLayerNames = m_ValidationLayers;

	// Thêm cấu hình debug messenger vào pNext của instance create info.
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	PopulateDebugMessengerCreateInfo(debugCreateInfo);
	instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_Handles.instance), "LỖI: Tạo Vulkan instance thất bại!");
}

void VulkanContext::CreateDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handles.instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		VK_CHECK(func(m_Handles.instance, &createInfo, nullptr, &m_Handles.debugMessenger), "LỖI: Tạo debug messenger thất bại!");
	}
	else
	{
		throw std::runtime_error("LỖI: Không tìm thấy extension vkCreateDebugUtilsMessengerEXT!");
	}
}

void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::string message = "Validation Layer: " + std::string(pCallbackData->pMessage);

	// Bỏ qua các thông báo cụ thể liên quan đến Steam overlay để tránh spam log.
	if (std::string(pCallbackData->pMessage).find("Failed to open JSON file E:\\Steam\\SteamOverlayVulkanLayer64.json") != std::string::npos ||
		std::string(pCallbackData->pMessage).find("Failed to open JSON file E:\\Steam\\SteamFossilizeVulkanLayer64.json") != std::string::npos)
	{
		return VK_FALSE; 
	}

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		Log::Error(message);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		Log::Warning(message);
	}

	return VK_FALSE;
}
void VulkanContext::CreateSurface(GLFWwindow* window)
{
	// Tạo một surface (bề mặt vẽ) từ cửa sổ GLFW.
	// Surface là cầu nối giữa Vulkan và hệ thống cửa sổ của HĐH.
	VK_CHECK(glfwCreateWindowSurface(m_Handles.instance, window, nullptr, &m_Handles.surface), "LỖI: Tạo window surface thất bại!");
}

void VulkanContext::ChoosePhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Handles.instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0) {
		throw std::runtime_error("LỖI: Không tìm thấy GPU nào hỗ trợ Vulkan!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_Handles.instance, &physicalDeviceCount, physicalDevices.data());

	// Duyệt qua tất cả các GPU có sẵn và chọn cái đầu tiên phù hợp.
	for (const auto& device : physicalDevices)
	{
		if (isPhysicalDeviceSuitable(device))
		{
			m_Handles.physicalDevice = device;

			VkPhysicalDeviceProperties deviceProperty{};
			vkGetPhysicalDeviceProperties(m_Handles.physicalDevice, &deviceProperty);
			//std::cout << "GPU được chọn: " << deviceProperty.deviceName << std::endl;
			break;
		}
	}

	if (m_Handles.physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("LỖỖI: Không tìm thấy GPU nào phù hợp yêu cầu!");
	}
}

void VulkanContext::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	const float queuePriority = 1.0f;

	// Sử dụng std::set để tự động loại bỏ các queue family index trùng lặp (ví dụ: graphic và present là cùng một queue).
	std::set<uint32_t> uniqueQueueFamilyIndices = { m_Handles.queueFamilyIndices.GraphicQueueIndex, m_Handles.queueFamilyIndices.PresentQueueIndex };
	
	for (uint32_t queueFamilyIndex : uniqueQueueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pQueuePriorities = &queuePriority;
		queueInfo.queueCount = 1;
		queueInfo.queueFamilyIndex = queueFamilyIndex;

		queueCreateInfos.push_back(queueInfo);
	}

	// Các tính năng của physical device mà chúng ta muốn sử dụng.
	VkPhysicalDeviceFeatures features{};
	features.fillModeNonSolid = VK_TRUE; // Cho phép vẽ wireframe
	features.samplerAnisotropy = VK_TRUE; // Cho phép lọc bất đẳng hướng

	// Feature Cho Dynamic Rendering
	// Feature không thuộc về physical như trên cần nối chuỗi qua pNext.
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFT{};;
	dynamicRenderingFT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFT.dynamicRendering = true;

	// Feature cho Descriptor Indexing (để hỗ trợ partially bound descriptors)
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
	descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeatures.pNext = &dynamicRenderingFT; // Nối chuỗi với dynamic rendering feature

	// Thông tin để tạo logical device.
	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensionsRequired.size());
	deviceInfo.ppEnabledExtensionNames = m_DeviceExtensionsRequired.data();
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.pEnabledFeatures = &features;
	deviceInfo.pNext = &descriptorIndexingFeatures; // Trỏ pNext chính vào descriptor indexing features

	VK_CHECK(vkCreateDevice(m_Handles.physicalDevice, &deviceInfo, nullptr, &m_Handles.device), "LỖI: Tạo logical device thất bại!");

	// Lấy handle của các queue từ logical device.
	vkGetDeviceQueue(m_Handles.device, m_Handles.queueFamilyIndices.GraphicQueueIndex, 0, &m_Handles.graphicQueue);
	vkGetDeviceQueue(m_Handles.device, m_Handles.queueFamilyIndices.PresentQueueIndex, 0, &m_Handles.presentQueue);
}

void VulkanContext::CreateVMAAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.device = m_Handles.device;
	allocatorInfo.instance = m_Handles.instance;
	allocatorInfo.physicalDevice = m_Handles.physicalDevice;

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_Handles.allocator), "LỖI: Tạo VMA Allocator thất bại!");
}

bool VulkanContext::isPhysicalDeviceSuitable(VkPhysicalDevice physDevice)
{
	// --- Tìm các queue family cần thiết --- 
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilyProperties.data());

	bool hasGraphicQueue = false;
	bool hasPresentQueue = false;

	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		// Tìm queue family hỗ trợ graphics.
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			hasGraphicQueue = true;
			m_Handles.queueFamilyIndices.GraphicQueueIndex = i;
		}

		// Tìm queue family hỗ trợ present lên surface.
		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, m_Handles.surface, &presentSupport);
		if (presentSupport)
		{
			hasPresentQueue = true;
			m_Handles.queueFamilyIndices.PresentQueueIndex = i;
		}

		if (hasGraphicQueue && hasPresentQueue)
		{
			break; // Đã tìm thấy đủ các queue cần thiết.
		}
	}

	// --- Kiểm tra các device extension được yêu cầu --- 
	uint32_t physExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &physExtensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(physExtensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &physExtensionCount, availableExtensions.data());

	// Dùng std::set để kiểm tra hiệu quả.
	std::set<std::string> requiredExtensions(m_DeviceExtensionsRequired.begin(), m_DeviceExtensionsRequired.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	bool hasExtensionsRequired = requiredExtensions.empty();

	return hasExtensionsRequired && hasGraphicQueue && hasPresentQueue;
}