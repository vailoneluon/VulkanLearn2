#pragma once

#include "Scene/LightData.h"
// --- Khai báo sớm (Forward Declarations) ---
// Giảm thiểu sự phụ thuộc vào các file header và tăng tốc độ biên dịch.

// Structs
struct UniformBufferObject;
struct PushConstantData;
class MaterialManager;
class LightManager;
class ShadowMapPass;
class Scene;
class Model;

// Classes
class Window;
class VulkanContext;
class VulkanSwapchain;
class VulkanCommandManager;
class VulkanSampler;
class VulkanDescriptorManager;
class VulkanPipeline;
class VulkanSyncManager;
class VulkanBuffer;
class VulkanImage;
class VulkanDescriptor;
class MeshManager;
class TextureManager;
class GeometryPass;
class BrightFilterPass;
class CompositePass;
class BlurPass;
class LightingPass;


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
	// =================================================================================================
	// SECTION: CẤU HÌNH VÀ TRẠNG THÁI 
	// =================================================================================================
	
	// --- Hằng số Cấu hình ---   
	const uint32_t WINDOW_WIDTH = 1200; 
	const uint32_t WINDOW_HEIGHT = 1000;
	//const VkClearColorValue BACKGROUND_COLOR = { 0.1f, 0.1f, 0.2f, 1.0f };
	const VkClearColorValue BACKGROUND_COLOR = { 0, 0, 0, 0 };
	const VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT; // Mức độ khử răng cưa (MSAA)
	const int MAX_FRAMES_IN_FLIGHT = 2; // Số lượng frame được xử lý đồng thời (double/triple buffering)
	const uint32_t MODEL_ROTATE_SPEED = 30;
	
	// --- Trạng thái Ứng dụng ---
	int m_CurrentFrame = 0; // Index của frame hiện tại đang được xử lý (từ 0 đến MAX_FRAMES_IN_FLIGHT - 1)

	// =================================================================================================
	// SECTION: CÁC ĐỐI TƯỢNG QUẢN LÝ CỐT LÕI
	// =================================================================================================

	// --- Quản lý Cửa sổ & Vulkan Core ---
	Window* m_Window;
	VulkanContext* m_VulkanContext;
	VulkanSwapchain* m_VulkanSwapchain;
	VulkanSampler* m_VulkanSampler;

	// --- Quản lý Tài nguyên & Đồng bộ hóa ---
	VulkanCommandManager* m_VulkanCommandManager;
	VulkanSyncManager* m_VulkanSyncManager;
	VulkanDescriptorManager* m_VulkanDescriptorManager;
	MeshManager* m_MeshManager;
	TextureManager* m_TextureManager;
	MaterialManager* m_MaterialManager;
	LightManager* m_LightManager;
	Scene* m_Scene;

	// =================================================================================================
	// SECTION: TÀI NGUYÊN RENDER (FRAMEBUFFER ATTACHMENTS)
	// =================================================================================================
	// Các ảnh (VulkanImage) này đóng vai trò là các attachment (đầu ra) cho các render pass.
	// Chúng được tạo cho mỗi frame-in-flight để tránh xung đột dữ liệu.

	// --- Geometry Pass (Vẽ scene 3D) ---
	std::vector<VulkanImage*> m_Geometry_DepthStencilImage;	// Attachment depth/stencil (MSAA) cho Geometry Pass.

	std::vector<VulkanImage*> m_Geometry_PositionImages;
	std::vector<VulkanImage*> m_Geometry_AlbedoImages;
	std::vector<VulkanImage*> m_Geometry_NormalImages;


	// --- Composite Pass (Tổng hợp cuối cùng) ---
	VulkanImage* m_Composite_ColorImage;						// Attachment màu (MSAA) cho Composite Pass.

	// --- Post-Processing (Bloom Effect) ---
	std::vector<VulkanImage*> m_LitSceneImages;
	std::vector<VulkanImage*> m_BrightImages;			// Chứa các vùng sáng được lọc từ m_SceneImages. Cũng là output của bước blur dọc.
	std::vector<VulkanImage*> m_TempBlurImages;			// Image tạm, là output của bước blur ngang và input cho bước blur dọc.

	// =================================================================================================
	// SECTION: DỮ LIỆU SCENE VÀ SHADER
	// =================================================================================================

	// --- Dữ liệu Scene ---
	entt::entity m_MainCamera;
	Model* m_AnimeGirlModel;	// Tài nguyên Model được tải một lần và dùng chung.
	entt::entity m_Girl1;		// Entity đại diện cho cô gái 1.
	entt::entity m_Girl2;		// Entity đại diện cho cô gái 2.



	// --- Dữ liệu Light
	entt::entity m_Light1;
	entt::entity m_Light2;

	// --- Dữ liệu cho Shader ---
	UniformBufferObject m_Geometry_Ubo{};					// Struct chứa dữ liệu cho Uniform Buffer (ma trận View, Projection).
	std::vector<VulkanBuffer*> m_Geometry_UniformBuffers;	// Các uniform buffer cho camera, một buffer cho mỗi frame-in-flight.

	// =================================================================================================
	// SECTION: CÁC RENDER PASS
	// =================================================================================================
	// Mỗi pass là một bước trong chuỗi render pipeline.

	GeometryPass* m_GeometryPass;			// Pass 1: Vẽ các đối tượng 3D vào một texture (render-to-texture).
	ShadowMapPass* m_ShadowMapPass;
	LightingPass* m_LightingPass;
	BrightFilterPass* m_BrightFilterPass;	// Pass 2: Lọc ra các vùng có độ sáng cao từ kết quả của Geometry Pass.
	BlurPass* m_BlurHPass;					// Pass 3: Áp dụng hiệu ứng mờ theo chiều ngang (Horizontal Blur).
	BlurPass* m_BlurVPass;					// Pass 4: Áp dụng hiệu ứng mờ theo chiều dọc (Vertical Blur).
	CompositePass* m_CompositePass;			// Pass 5: Tổng hợp kết quả của Geometry Pass và các bước blur để tạo ra ảnh cuối cùng và hiển thị lên màn hình.
	 
	// =================================================================================================
	// SECTION: CÁC HÀM HELPER
	// =================================================================================================

private:
	// --- Nhóm hàm khởi tạo ---
	void CreateSceneLights();
	void CreateRenderPasses();
	void CreateFrameBufferImages();
	void CreateUniformBuffers();

	// --- Nhóm hàm cập nhật mỗi frame ---
	void Update_Geometry_Uniforms();
	void UpdateRenderObjectTransforms();

	// --- Nhóm hàm vẽ và ghi command buffer ---
	void DrawFrame();
	void RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex);
};