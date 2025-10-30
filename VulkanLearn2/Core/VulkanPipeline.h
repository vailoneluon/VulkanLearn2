#pragma once
#include "VulkanContext.h"
#include <string>
#include <vector>
#include <map>

// Forward declarations
struct RenderPassHandles;
struct SwapchainHandles;
class VulkanDescriptor;

// Struct chứa các handle nội bộ của VulkanPipeline.
struct PipelineHandles
{
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
};

// Class quản lý việc tạo ra một Graphics Pipeline hoàn chỉnh.
// Nó bao gồm việc đọc shader, tạo pipeline layout và cấu hình tất cả các giai đoạn
// của pipeline (vertex input, rasterization, color blending, v.v.).
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

	// Lấy các handle nội bộ.
	const PipelineHandles& getHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	PipelineHandles m_Handles;

	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Hàm helper private ---

	// Đọc nội dung file shader (mã SPIR-V).
	static std::vector<char> ReadShaderFile(const std::string& filename);

	// Tạo một VkShaderModule từ mã SPIR-V đã đọc.
	VkShaderModule CreateShaderModule(const std::string& shaderFilePath);

	// Tạo pipeline layout từ danh sách các descriptor set layout.
	void CreatePipelineLayout(const std::vector<VulkanDescriptor*>& descriptors);

	// Hàm chính để tạo Graphics Pipeline.
	void CreateGraphicsPipeline(
		const RenderPassHandles& renderPassHandles,
		const SwapchainHandles& swapchainHandles,
		VkSampleCountFlagBits msaaSamples,
		VkShaderModule vertShaderModule,
		VkShaderModule fragShaderModule
	);
};