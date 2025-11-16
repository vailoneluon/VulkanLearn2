#pragma once

#include "Core/VulkanContext.h"
#include "Utils/ModelLoader.h"
#include "Renderer/BrightFilterPass.h"
#include "Renderer/CompositePass.h"
#include "Renderer/BlurPass.h"

// --- Khai báo sớm (Forward Declarations) ---
// Giảm thiểu sự phụ thuộc vào các file header và tăng tốc độ biên dịch.
class Window;
class VulkanSwapchain;

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
class GeometryPass;


/**
 * @class Application
 * @brief Lớp chính điều phối toàn bộ ứng dụng Vulkan, quản lý vòng lặp render và các tài nguyên.
 */
class Application
{
public:
	/**
	 * @brief Hàm khởi tạo, thiết lập toàn bộ môi trường Vulkan.
	 */
	Application();

	/**
	 * @brief Hàm hủy, giải phóng tất cả các tài nguyên đã được cấp phát.
	 */
	~Application();

	/**
	 * @brief Bắt đầu vòng lặp render chính của ứng dụng.
	 */
	void Loop();

private:
	// --- Hằng số & Cấu hình ---
	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;
	const VkClearColorValue BACKGROUND_COLOR = { 0.1f, 0.1f, 0.2f, 1.0f };
	const VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_4_BIT; // Mức độ khử răng cưa (MSAA)
	const int MAX_FRAMES_IN_FLIGHT = 2; // Số lượng frame được xử lý đồng thời (double buffering)

	// --- Trạng thái Ứng dụng ---
	int m_CurrentFrame = 0; // Index của frame hiện tại đang được xử lý

	// --- Quản lý Cửa sổ ---
	Window* m_Window;

	// --- Thành phần Vulkan Cốt lõi ---
	VulkanContext* m_VulkanContext;
	VulkanSwapchain* m_VulkanSwapchain;

	VulkanSampler* m_VulkanSampler;

	// --- Quản lý Tài nguyên Vulkan ---
	VulkanCommandManager* m_VulkanCommandManager;
	VulkanSyncManager* m_VulkanSyncManager;
	VulkanDescriptorManager* m_VulkanDescriptorManager;
	MeshManager* m_MeshManager;
	TextureManager* m_TextureManager;

	// --- Tài nguyên ảnh cho các bước render (Attachments) ---


	// Images dùng làm attachment cho các bước render
	// - RTT Pass (Render-To-Texture)
	//VulkanImage* m_RTT_ColorImage;
	std::vector<VulkanImage*> m_RTT_ColorImage;
	std::vector<VulkanImage*> m_RTT_DepthStencilImage;
	std::vector<VulkanImage*> m_SceneImages; // Kết quả của RTT pass, dùng làm input cho các pass sau
	// - Main Pass
	VulkanImage* m_Main_ColorImage;
	VulkanImage* m_Main_DepthStencilImage;
	// - Bright Pass
	std::vector<VulkanImage*> m_BrightImages;	// Kết quả của Bright pass, chứa các vùng sáng
	// - Blur Pass
	std::vector<VulkanImage*> m_TempBlurImages; // Image tạm thời cho bước blur ngang


	// --- Dữ liệu Scene ---
	RenderObject* m_BunnyGirl;
	RenderObject* m_Swimsuit;
	std::vector<RenderObject*> m_RenderObjects; // Danh sách các đối tượng cần render

	// --- Descriptors & Dữ liệu Shader ---
	UniformBufferObject m_RTT_Ubo{}; // Struct chứa dữ liệu cho Uniform Buffer (View/Projection Matrix)
	std::vector<VulkanBuffer*> m_RTT_UniformBuffers; // Uniform buffers cho mỗi frame-in-flight

	// Descriptors cho các pipeline khác nhau
	std::vector<VulkanDescriptor*> m_RTT_UniformDescriptors;

	std::vector<VulkanDescriptor*> m_MainTextureDescriptors;

	std::vector<VulkanDescriptor*> m_BrightTextureDescriptors;

	std::vector<VulkanDescriptor*> m_BlurHTextureDescriptors;

	std::vector<VulkanDescriptor*> m_BlurVTextureDescriptors;

	std::vector<VulkanDescriptor*> m_allDescriptors; // Tổng hợp tất cả descriptors để quản lý

	PushConstantData m_PushConstantData; // Struct chứa dữ liệu cho Push Constants (Model Matrix, Texture ID)


	GeometryPass* m_GeometryPass;
	BrightFilterPass* m_BrightFilterPass;
	CompositePass* m_CompositePass;
	BlurPass* m_BlurHPass;
	BlurPass* m_BlurVPass;


	// --- Các hàm Private ---

	// --- Nhóm hàm khởi tạo ---
	void CreateRenderPasses();
	void CreateFrameBufferImages();
	void CreateUniformBuffers();
	void CreateMainDescriptors();
	void CreateBrightDescriptors();
	void CreateBlurHDescriptors();
	void CreateBlurVDescriptors();

	// --- Nhóm hàm cập nhật mỗi frame ---
	void UpdateRTT_Uniforms();
	void UpdateRenderObjectTransforms();

	// --- Nhóm hàm vẽ và ghi command buffer ---
	void DrawFrame();
	void RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex);

};