#pragma once
#include "VulkanContext.h"

struct RenderPassHandles;
struct SwapchainHandles;
class VulkanDescriptor;
//#include "VulkanRenderPass.h"
//#include "VulkanSwapchain.h"
//#include "VulkanDescriptor.h"


struct PipelineHandles
{
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
};

class VulkanPipeline
{
public:
	VulkanPipeline(
		const VulkanHandles& vulkanHandles,
		const RenderPassHandles& renderPassHandles,
		const SwapchainHandles& swapchainHandles,
		VkSampleCountFlagBits msaaSamples,
		vector<VulkanDescriptor*>& descriptors
	);
	~VulkanPipeline();

	const PipelineHandles& getHandles() const { return handles; }

private:
	PipelineHandles handles;
	const VulkanHandles& vk;

	// Hàm helper để đọc file shader
	static vector<char> readShaderFile(const string& filename);

	// Hàm helper tạo VkShaderModule
	VkShaderModule createShaderModule(const vector<char>& code);

	void createPipelineLayout(vector<VkDescriptorSetLayout>& descSetLayouts);

	// Hàm tạo Pipeline chính
	void createGraphicsPipeline(
		const RenderPassHandles& renderPassHandles,
		const SwapchainHandles& swapchainHandles,
		VkSampleCountFlagBits msaaSamples,
		VkShaderModule vertShaderModule,
		VkShaderModule fragShaderModule
	);
};