#pragma once
#include "VulkanContext.h"
#include <string>
#include <vector>
#include <map>

// Forward declarations

struct SwapchainHandles;
class VulkanDescriptor;

// =================================================================================================
// Struct: PipelineHandles
// Mô tả: Chứa các handle nội bộ của VulkanPipeline.
//        Bao gồm VkPipelineLayout và VkPipeline.
// =================================================================================================
struct PipelineHandles
{
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
};

// =================================================================================================
// Struct: VulkanPipelineCreateInfo
// Mô tả: Chứa thông tin cần thiết để khởi tạo một Graphics Pipeline.
//        Bao gồm shader paths, descriptors, render attachments, viewport, depth/stencil settings, v.v.
// =================================================================================================
struct VulkanPipelineCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* swapchainHandles;
	VkSampleCountFlagBits msaaSamples;
	std::vector<VulkanDescriptor*>* descriptors;
	std::vector<VkFormat>* renderingColorAttachments;

	std::string vertexShaderFilePath;
	std::string fragmentShaderFilePath;

	bool useVertexInput = true;

	VkExtent2D viewportExtent = {0, 0};
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	VkFormat stencilFormat = VK_FORMAT_UNDEFINED;

	bool enableDepthBias = false;
	VkCullModeFlags cullingMode = VK_CULL_MODE_BACK_BIT;
	VkDeviceSize pushConstantDataSize = sizeof(PushConstantData);
};


// =================================================================================================
// Class: VulkanPipeline
// Mô tả: 
//      Quản lý việc tạo ra một Graphics Pipeline hoàn chỉnh.
//      Bao gồm việc đọc shader, tạo pipeline layout và cấu hình tất cả các giai đoạn
//      của pipeline (vertex input, rasterization, color blending, v.v.).
// =================================================================================================
class VulkanPipeline
{
public:
	// Constructor: Khởi tạo Graphics Pipeline.
	// Tham số:
	//      pipelineInfo: Con trỏ đến struct chứa thông tin khởi tạo.
	VulkanPipeline(const VulkanPipelineCreateInfo* pipelineInfo);

	// Destructor: Hủy pipeline và layout.
	~VulkanPipeline();

	// Getter: Lấy các handle nội bộ.
	const PipelineHandles& getHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	PipelineHandles m_Handles;

	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Hàm helper private ---

	// Helper: Đọc nội dung file shader (mã SPIR-V).
	static std::vector<char> ReadShaderFile(const std::string& filename);

	// Helper: Tạo một VkShaderModule từ mã SPIR-V đã đọc.
	VkShaderModule CreateShaderModule(const std::string& shaderFilePath);

	// Helper: Tạo pipeline layout từ danh sách các descriptor set layout.
	void CreatePipelineLayout(const std::vector<VulkanDescriptor*>& descriptors, VkDeviceSize pushConstantDataSize);

	// Helper: Hàm chính để tạo Graphics Pipeline.
	void CreateGraphicsPipeline(
		const VulkanPipelineCreateInfo* pipelineInfo,
		VkShaderModule vertShaderModule,
		VkShaderModule fragShaderModule
	);
};