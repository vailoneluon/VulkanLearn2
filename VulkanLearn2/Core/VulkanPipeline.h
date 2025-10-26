#pragma once
#include "VulkanContext.h"

struct RenderPassHandles;
struct SwapchainHandles;
class VulkanDescriptor;


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
		std::vector<VulkanDescriptor*>& descriptors
	);
	~VulkanPipeline();

	const PipelineHandles& getHandles() const { return handles; }

private:
	PipelineHandles handles;
	const VulkanHandles& vk;

	// Hàm helper để đọc file shader
	static std::vector<char> readShaderFile(const std::string& filename);

	// Hàm helper tạo VkShaderModule
	VkShaderModule createShaderModule(const std::string& shaderFilePath);

	void createPipelineLayout(const std::vector<VulkanDescriptor*>& descriptors);

	// Hàm tạo Pipeline chính
	void createGraphicsPipeline(
		const RenderPassHandles& renderPassHandles,
		const SwapchainHandles& swapchainHandles,
		VkSampleCountFlagBits msaaSamples,
		VkShaderModule vertShaderModule,
		VkShaderModule fragShaderModule
	);
};