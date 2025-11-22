#include "pch.h"
#include "LightManager.h"
#include "Core/VulkanBuffer.h"
#include "Core/VulkanDescriptor.h"
#include "Core\VulkanCommandManager.h"

LightManager::LightManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, std::vector<Light>* allSceneLights, uint32_t maxFramesInFlight):
	m_VulkanHandles(vulkanHandles),
	m_CommandManager(commandManager)
{
	for (const auto& light : *allSceneLights)
	{
		m_AllSceneGpuLights.push_back(light.ToGPU());	
	}

	CreateLightBuffers(maxFramesInFlight);
	CreateDescriptors(maxFramesInFlight);
}

LightManager::~LightManager()
{
	for (const auto& buffer : m_SceneLightBuffers)
	{
		delete(buffer);
	}
}

void LightManager::CreateLightBuffers(uint32_t maxFramesInFlight)
{
	m_SceneLightBuffers.resize(maxFramesInFlight);

	VkDeviceSize bufferSize = sizeof(GPULight) * m_AllSceneGpuLights.size();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.size = bufferSize;

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		m_SceneLightBuffers[i] = new VulkanBuffer(m_VulkanHandles, m_CommandManager, bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY);
		m_SceneLightBuffers[i]->UploadData(m_AllSceneGpuLights.data(), bufferSize, 0);
	}
}

void LightManager::CreateDescriptors(uint32_t maxFramesInFlight)
{
	m_LightBufferDescriptors.resize(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_SceneLightBuffers[i]->GetHandles().buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = m_SceneLightBuffers[i]->GetHandles().bufferSize;

		BufferDescriptorUpdateInfo bufferUpdateInfo{};
		bufferUpdateInfo.binding = 0;
		bufferUpdateInfo.firstArrayElement = 0;
		bufferUpdateInfo.bufferInfos = { bufferInfo };
		
		BindingElementInfo elementInfo{};
		elementInfo.binding = 0;
		elementInfo.descriptorCount = 1;
		elementInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		elementInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		elementInfo.bufferDescriptorUpdateInfoCount = 1;
		elementInfo.pBufferDescriptorUpdates = &bufferUpdateInfo;

		m_LightBufferDescriptors[i] = new VulkanDescriptor(m_VulkanHandles, {elementInfo}, 1);
	}
}
