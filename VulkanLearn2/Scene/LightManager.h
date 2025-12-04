#pragma once
#include "LightData.h"
#include "Core\VulkanContext.h"
#include "Core\VulkanSampler.h"

class VulkanBuffer;
class VulkanDescriptor;
class VulkanCommandManager;
class VulkanImage;
class Scene;

// =================================================================================================
// Class: LightManager
// Mô tả: 
//      Quản lý tất cả các nguồn sáng trong scene.
//      Chịu trách nhiệm tạo buffer ánh sáng (SSBO), shadow maps, và descriptor sets liên quan.
// =================================================================================================
class LightManager
{
public:
	// Constructor: Khởi tạo LightManager với danh sách đèn ban đầu.
	LightManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, Scene* scene, const VulkanSampler* sampler, uint32_t maxFramesInFlight);
	~LightManager();

	// --- Getters ---
	const std::vector<VulkanDescriptor*>& GetDescriptors() const { return m_LightBufferDescriptors; }
	const std::vector<GPULight>& GetAllGpuLights(uint32_t currentFrame) const { return m_AllSceneGpuLights; }
	const std::vector<VulkanImage*>& GetShadowMappingImage(uint32_t currentFrame) const { return m_ShadowMappingImages[currentFrame]; }
	const uint32_t GetShadowSize() const { return SHADOW_SIZE; }

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;
	VulkanCommandManager* m_CommandManager;
	Scene* m_Scene;
	const VulkanSampler* m_VulkanSampler;

	uint32_t m_MaxFramesInFlight;

	// --- Cấu hình Shadow Map ---
	const uint32_t SHADOW_SIZE = 2048;
	const uint32_t MAX_SHADOW_DESCRIPTOR = 256;

	// --- Dữ liệu nội bộ ---
	std::vector<GPULight> m_AllSceneGpuLights;
	std::vector<VulkanBuffer*> m_SceneLightBuffers;
	std::vector<VulkanDescriptor*> m_LightBufferDescriptors;

	std::vector<std::vector<VulkanImage*>> m_ShadowMappingImages;
	VulkanImage* m_DummyShadowMap = nullptr; // Placeholder khi không có shadow map nào được tạo hoặc có ít hơn MAX_SHADOW_DESCRIPTOR


	// --- Hàm helper private ---
	void CreateLightBuffers(uint32_t maxFramesInFlight);
	void CreateShadowMappingTexture(uint32_t maxFramesInFlight);
	void CreateDummyShadowMap(); // Tạo một ảnh depth 1x1 làm placeholder
	void CreateDescriptors(uint32_t maxFramesInFlight);

	void UpdateLightSpaceMatrices();
	void UploadLightData(uint32_t currentFrame);

};
