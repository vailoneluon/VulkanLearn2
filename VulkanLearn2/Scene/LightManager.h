#pragma once
#include "LightData.h"
#include "Core\VulkanContext.h"
#include "Core\VulkanSampler.h"

class VulkanBuffer;
class VulkanDescriptor;
class VulkanCommandManager;
class VulkanImage;

class LightManager
{
public:
	LightManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, const VulkanSampler* sampler, std::vector<Light>* allSceneLights, uint32_t maxFramesInFlight);
	~LightManager();

	const std::vector<VulkanDescriptor*>& GetDescriptors() const { return m_LightBufferDescriptors; }
	const std::vector<GPULight>& GetAllGpuLights(uint32_t currentFrame) const { return m_AllSceneGpuLights; }
	const std::vector<VulkanImage*>& GetShadowMappingImage(uint32_t currentFrame) const { return m_ShadowMappingImages[currentFrame]; }
	const uint32_t GetShadowSize() const { return SHADOW_SIZE; }


private:
	const VulkanHandles& m_VulkanHandles;
	VulkanCommandManager* m_CommandManager;
	const VulkanSampler* m_VulkanSampler;

	uint32_t m_MaxFramesInFlight;

	const uint32_t SHADOW_SIZE = 1024;
	const uint32_t MAX_SHADOW_DESCRIPTOR = 256;

	std::vector<GPULight> m_AllSceneGpuLights;
	std::vector<VulkanBuffer*> m_SceneLightBuffers;
	std::vector<VulkanDescriptor*> m_LightBufferDescriptors;

	std::vector<std::vector<VulkanImage*>> m_ShadowMappingImages;
	VulkanImage* m_DummyShadowMap = nullptr; // Placeholder khi không có shadow map nào được tạo hoặc có ít hơn MAX_SHADOW_DESCRIPTOR


	void CreateLightBuffers(uint32_t maxFramesInFlight);
	void CreateShadowMappingTexture(uint32_t maxFramesInFlight);
	void CreateDummyShadowMap(); // Tạo một ảnh depth 1x1 làm placeholder
	void CreateDescriptors(uint32_t maxFramesInFlight);

	void UpdateLightSpaceMatrices();
	void UploadLightData(uint32_t currentFrame);

};
