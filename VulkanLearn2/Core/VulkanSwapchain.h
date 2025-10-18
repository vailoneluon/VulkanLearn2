#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
using namespace std;

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> presentModes;

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

	vector<VkImageView> swapchainImageViews;
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
