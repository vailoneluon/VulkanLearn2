#include "pch.h"
#include "VulkanContext.h"


VulkanContext::VulkanContext(GLFWwindow* window, std::vector<const char*> instanceExtensions)
{
	CreateInstance(instanceExtensions);
	CreateSurface(window);
	ChoosePhysicalDevice();
	CreateLogicalDevice();
}

VulkanContext::~VulkanContext()
{
	// Sử dụng handles.instance và handles.surface
	vkDestroySurfaceKHR(handles.instance, handles.surface, nullptr);

	// Sử dụng handles.device và handles.instance
	vkDestroyDevice(handles.device, nullptr);
	vkDestroyInstance(handles.instance, nullptr);
}

void VulkanContext::CreateInstance(std::vector<const char*> extensions)
{
	instanceExtensionsRequired.insert(instanceExtensionsRequired.end(), extensions.begin(), extensions.end());

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName = "ZOLCOL ENGINE";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensionsRequired.size());
	instanceInfo.ppEnabledExtensionNames = instanceExtensionsRequired.data();
	instanceInfo.enabledLayerCount = 1;
	instanceInfo.ppEnabledLayerNames = validationLayers;

	// Gán kết quả vào handles.instance
	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &handles.instance), "FAILED TO CREATE VULKAN INSTANCE");
}

void VulkanContext::CreateSurface(GLFWwindow* window)
{
	// Sử dụng handles.instance và gán kết quả cho handles.surface
	VK_CHECK(glfwCreateWindowSurface(handles.instance, window, nullptr, &handles.surface), "FAILED TO CREATE WINDOW SURFACE");
}

void VulkanContext::ChoosePhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	// Sử dụng handles.instance
	vkEnumeratePhysicalDevices(handles.instance, &physicalDeviceCount, nullptr);

	std::vector<VkPhysicalDevice> physicalDevices;
	physicalDevices.resize(physicalDeviceCount);
	// Sử dụng handles.instance
	vkEnumeratePhysicalDevices(handles.instance, &physicalDeviceCount, physicalDevices.data());

	for (int i = 0; i < physicalDeviceCount; i++)
	{
		if (isPhysicalDeviceSuitable(physicalDevices[i]))
		{
			// Gán vào handles.physicalDevice
			handles.physicalDevice = physicalDevices[i];

			VkPhysicalDeviceProperties deviceProperty{};
			// Sử dụng handles.physicalDevice
			vkGetPhysicalDeviceProperties(handles.physicalDevice, &deviceProperty);
			std::cout << "Selected GPU: " << deviceProperty.deviceName << std::endl;

			break;
		}
	}

	// Kiểm tra handles.physicalDevice
	if (handles.physicalDevice == VK_NULL_HANDLE)
	{
		showError("FAILED TO CHOOSE PHYSICAL DEVICE");
	}
}

void VulkanContext::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	const float queuePriority = 1.0f;

	std::set<uint32_t> queueFamilyIndicesSet = { handles.queueFamilyIndices.GraphicQueueIndex, handles.queueFamilyIndices.PresentQueueIndex };
	for (uint32_t queueFamily : queueFamilyIndicesSet)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pQueuePriorities = &queuePriority;
		queueInfo.queueCount = 1;
		queueInfo.queueFamilyIndex = queueFamily;

		queueCreateInfos.push_back(queueInfo);
	}

	VkPhysicalDeviceFeatures features{};
	features.fillModeNonSolid = VK_TRUE;
	features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(physicalRequireDeviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = physicalRequireDeviceExtensions.data();
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.pEnabledFeatures = &features;

	// Sử dụng handles.physicalDevice và gán kết quả cho handles.device
	VK_CHECK(vkCreateDevice(handles.physicalDevice, &deviceInfo, nullptr, &handles.device), "FAILED TO CREATE LOGICAL DEVICE");

	// Sử dụng handles.device
	vkGetDeviceQueue(handles.device, handles.queueFamilyIndices.GraphicQueueIndex, 0, &handles.graphicQueue);
	vkGetDeviceQueue(handles.device, handles.queueFamilyIndices.PresentQueueIndex, 0, &handles.presentQueue);
}

bool VulkanContext::isPhysicalDeviceSuitable(VkPhysicalDevice physDevice)
{
	bool hasGraphicQueue = false;
	bool hasPresentQueue = false;
	bool hasExtensionsRequired = false;

	// Check have Graphic and Present Queue
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilyProperties.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			hasGraphicQueue = true;
			handles.queueFamilyIndices.GraphicQueueIndex = i;
			// Không cần break ở đây để tìm queue family tốt nhất (nếu cần)
		}

		VkBool32 presentSupport = VK_FALSE;
		// Sử dụng handles.surface
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, handles.surface, &presentSupport);
		if (presentSupport)
		{
			hasPresentQueue = true;
			handles.queueFamilyIndices.PresentQueueIndex = i;
		}

		if (hasGraphicQueue && hasPresentQueue)
		{
			// Có thể break ở đây nếu bạn chỉ cần tìm cặp đầu tiên
			break;
		}
	}

	// Kiểm tra extension có hỗ trợ không.
	uint32_t physExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &physExtensionCount, nullptr);

	std::vector<VkExtensionProperties> deviceExtensions(physExtensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &physExtensionCount, deviceExtensions.data());

	std::set<std::string> requiredExtensions(physicalRequireDeviceExtensions.begin(), physicalRequireDeviceExtensions.end());

	for (const auto& extension : deviceExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	hasExtensionsRequired = requiredExtensions.empty();

	return hasExtensionsRequired && hasGraphicQueue && hasPresentQueue;
}