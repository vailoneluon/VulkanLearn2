#pragma once

#include "Core/VulkanContext.h"
#include "Core/VulkanTypes.h"
#include "Utils/ModelLoader.h"

// Khai báo sớm (forward declarations) để giảm phụ thuộc header và tăng tốc độ biên dịch.
class Window;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanFrameBuffer;
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
	VulkanFrameBuffer* m_VulkanFrameBuffer;
	VulkanPipeline* m_VulkanPipeline;
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
	UniformBufferObject m_Ubo{}; // Uniform Buffer Object (UBO) cho ma trận view/projection
	std::vector<VulkanBuffer*> m_UniformBuffers; // Một uniform buffer cho mỗi frame đang xử lý
	std::vector<VulkanDescriptor*> m_UniformDescriptors; // Một descriptor cho mỗi uniform buffer
	std::vector<VulkanDescriptor*> m_PipelineDescriptors; // Tất cả descriptor được sử dụng bởi pipeline

	// --- Instance Buffer
	const uint32_t instanceCount = 3;
	std::vector<InstanceData> instanceDatas;
	std::vector<VulkanBuffer*> m_InstanceBuffers;
	std::vector<VulkanBuffer*> m_InstanceSBOBuffers;
	std::vector<VulkanDescriptor*> m_InstanceSBODescriptor;

	PushConstantData m_PushConstantData; // Dữ liệu cho push constants (ví dụ: ma trận model)

	// --- Các hàm private ---

	// Các hàm hỗ trợ khởi tạo
	void CreateUniformBuffers();
	void CreateInstanceBuffers();
	void CreateInstanceSBOBuffers();
	void UpdateDescriptorBindings();

	// Cập nhật mỗi frame
	void UpdateUniforms();
	void UpdateRenderObjectTransforms();
	

	// Vẽ
	void DrawFrame();
	void RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex);
	void CmdDrawRenderObjects(const VkCommandBuffer& cmdBuffer);
	void BindDescriptorSets(const VkCommandBuffer& cmdBuffer);
};