#pragma once

#include "Core/VulkanContext.h"
#include "Utils/ModelLoader.h"

// Khai báo sớm (forward declarations) để giảm phụ thuộc header và tăng tốc độ biên dịch.
class Window;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanCommandManager;
class VulkanSampler;
class VulkanDescriptorManager;
class VulkanPipeline;
class VulkanSyncManager;
class VulkanBuffer;
class VulkanImage;
class VulkanDescriptor;
class RenderObject;
class MeshManager;
class TextureManager;
class VulkanFrameBuffer;

// Class Application chính, điều phối toàn bộ quá trình render.
class Application
{
public:
	Application();
	~Application();

	// Chạy vòng lặp chính của ứng dụng.
	void Loop();

private:
	// --- Hằng số ---
	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;
	const VkClearColorValue BACKGROUND_COLOR = { 0.1f, 0.1f, 0.2f, 1.0f };
	const VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_4_BIT; // Mức độ khử răng cưa (MSAA)
	const int MAX_FRAMES_IN_FLIGHT = 2; // Sử dụng double buffering

	// --- Trạng thái cốt lõi của ứng dụng ---
	int m_CurrentFrame = 0; // Index của frame hiện tại đang được xử lý

	// --- Quản lý cửa sổ ---
	Window* m_Window;

	// --- Các thành phần Vulkan cốt lõi ---
	VulkanContext* m_VulkanContext;
	VulkanSwapchain* m_VulkanSwapchain;
	VulkanRenderPass* m_VulkanRenderPass;

	// Frame Buffer
	std::vector<VulkanFrameBuffer*> m_RTT_FrameBuffers;
	std::vector<VulkanFrameBuffer*> m_Main_FrameBuffers;
	std::vector<VulkanFrameBuffer*> m_Bright_FrameBuffers;

	// Vulkan Image For Frame Buffers
		// - RTT Frame Buffer
	VulkanImage* m_RTT_ColorImage;
	VulkanImage* m_RTT_DepthStencilImage;
	std::vector<VulkanImage*> m_SceneImages;
		// - Main Frame Buffer
	VulkanImage* m_Main_ColorImage;
	VulkanImage* m_Main_DepthStencilImage;
		// - Bright Pass Frame Buffer
	std::vector<VulkanImage*> m_BrightImages;	// output của bright renderpass

	VulkanPipeline* m_RTTVulkanPipeline;
	VulkanPipeline* m_MainVulkanPipeline;
	VulkanPipeline* m_BrightVulkanPipeline;
	VulkanSampler* m_VulkanSampler;

	// --- Các trình quản lý Vulkan ---
	VulkanCommandManager* m_VulkanCommandManager;
	VulkanSyncManager* m_VulkanSyncManager;
	VulkanDescriptorManager* m_VulkanDescriptorManager;
	MeshManager* m_MeshManager;
	TextureManager* m_TextureManager;

	// --- Dữ liệu Scene ---
	RenderObject* m_BunnyGirl;
	RenderObject* m_Swimsuit;
	std::vector<RenderObject*> m_RenderObjects; // Tất cả object cần được render trong scene

	// --- Uniforms, Descriptors và Push Constants ---
	UniformBufferObject m_RTT_Ubo{}; // Uniform Buffer Object (UBO) cho ma trận view/projection
	std::vector<VulkanBuffer*> m_RTT_UniformBuffers; // Một uniform buffer cho mỗi frame đang xử lý

		// Descriptor Set Cho Frame Buffer
		// PipelineDescriptor chỉ để truyền vào khởi tạo PipelineLayout
	std::vector<VulkanDescriptor*> m_RTTPipelineDescriptors; // Tất cả descriptor được sử dụng bởi pipeline
	std::vector<VulkanDescriptor*> m_RTT_UniformDescriptors; // Một descriptor cho mỗi uniform buffer

	std::vector<VulkanDescriptor*> m_MainPipelineDescriptors;
	std::vector<VulkanDescriptor*> m_MainTextureDescriptors;

	std::vector<VulkanDescriptor*> m_BrightPipelineDescriptors;
	std::vector<VulkanDescriptor*> m_BrightTextureDescriptors;
	
	std::vector<VulkanDescriptor*> m_allDescriptors;

	PushConstantData m_PushConstantData; // Dữ liệu cho push constants (ví dụ: ma trận model)

	// --- Các hàm private ---

	// Các hàm hỗ trợ khởi tạo
	void CreateFrameBufferImages();
	void CreateFrameBuffers();

	void CreateUniformBuffers();
	void CreateMainDescriptors();
	void CreateBrightDescriptors();

	void CreatePipelines();
	void UpdateRTTDescriptorBindings();
	void UpdateMainDescriptorBindings();
	void UpdateBrightDescriptorBindings();

	// Cập nhật mỗi frame
	void UpdateRTT_Uniforms();
	void UpdateRenderObjectTransforms();

	// Vẽ
	void DrawFrame();
	void RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex);
	
	void CmdDrawRTTRenderPass(const VkCommandBuffer& cmdBuffer);
	void CmdDrawBrightRenderPass(const VkCommandBuffer& cmdBuffer);
	void CmdDrawMainRenderPass(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex);


	void CmdDrawRTTRenderObjects(const VkCommandBuffer& cmdBuffer);
	void CmdDrawBright(const VkCommandBuffer& cmdBuffer);
	void CmdDrawMain(const VkCommandBuffer& cmdBuffer);
	
	void BindRTTDescriptorSets(const VkCommandBuffer& cmdBuffer);
	void BindMainDescriptorSets(const VkCommandBuffer& cmdBuffer);
	void BindBrightDescriptorSets(const VkCommandBuffer& cmdBuffer);
};