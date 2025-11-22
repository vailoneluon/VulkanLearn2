#pragma once
#include "LightData.h"
#include "Core\VulkanContext.h"

class VulkanBuffer;
class VulkanDescriptor;
class VulkanCommandManager;

class LightManager
{
public:
	LightManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, std::vector<Light>* allSceneLights, uint32_t maxFramesInFlight);
	~LightManager();

	const std::vector<VulkanDescriptor*>& GetDescriptors() const { return m_LightBufferDescriptors; }
private:
	const VulkanHandles& m_VulkanHandles;
	VulkanCommandManager* m_CommandManager;

	std::vector<GPULight> m_AllSceneGpuLights;
	std::vector<VulkanBuffer*> m_SceneLightBuffers;
	std::vector<VulkanDescriptor*> m_LightBufferDescriptors;

	void CreateLightBuffers(uint32_t maxFramesInFlight);
	void CreateDescriptors(uint32_t maxFramesInFlight);
};
