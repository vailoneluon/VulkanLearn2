#pragma once
#include "VulkanContext.h"

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	VkSurfaceFormatKHR chosenFormat;
	VkPresentModeKHR chosenPresentMode;

	void ChooseSurfaceFormatKHR();
	void ChoosePresentModeKHR();
};

struct SwapchainHandles
{
	uint32_t swapchainImageCount;

	VkSwapchainKHR swapchain;
	SwapchainSupportDetails swapchainSuportDetails;
	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapchainImageViews;
};

class VulkanSwapchain
{
public:
	VulkanSwapchain(const VulkanHandles& vulkanHandles,GLFWwindow* window);
	~VulkanSwapchain();

	const SwapchainHandles& getHandles() const { return handles; }

private:
	const VulkanHandles& vk;
	GLFWwindow* window;
	
	SwapchainHandles handles;

	void querrySwapchainSupportDetails(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
	void chooseSwapchainExtent();

	void CreateSwapchain();
	void CreateSwapchainImageview();
};
