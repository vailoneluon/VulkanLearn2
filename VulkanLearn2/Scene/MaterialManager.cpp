#include "pch.h"
#include "MaterialManager.h"

#include "Core\VulkanCommandManager.h"
#include "TextureManager.h"
#include "Core\VulkanDescriptor.h"


MaterialManager::MaterialManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, TextureManager* textureManager):
	m_VulkanHandles(vulkanHandles),
	m_CommandManager(commandManager),
	m_TextureManager(textureManager)
{

}

MaterialManager::~MaterialManager()
{
	delete(m_MaterialBuffer);
}

uint32_t MaterialManager::LoadMaterial(const MaterialRawData& materialRawData)
{
	MaterialData material{};
	if (materialRawData.diffuseMapFileName != "")
	{
		material.diffuseMapIndex = m_TextureManager->LoadTextureImage(materialRawData.diffuseMapFileName);
	}
	if (materialRawData.normalMapFileName != "")
	{
		material.normalMapIndex = m_TextureManager->LoadTextureImage(materialRawData.normalMapFileName);
	}
	if (materialRawData.specularMapFileName != "")
	{
		material.specularMapIndex = m_TextureManager->LoadTextureImage(materialRawData.specularMapFileName);
	}

	m_Handles.allMaterials.push_back(material);
	return m_Handles.allMaterials.size() - 1;
}

VulkanDescriptor* MaterialManager::GetDescriptor()
{
	return m_Handles.descriptor;
}

void MaterialManager::Finalize()
{
	CreateMaterialBuffer();
	CreateMaterialDescriptor();
}

void MaterialManager::CreateMaterialBuffer()
{
	VkDeviceSize bufferSize = sizeof(MaterialData) * m_Handles.allMaterials.size();

	if (bufferSize == 0)
	{
		return;
	}

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	m_MaterialBuffer = new VulkanBuffer(m_VulkanHandles, m_CommandManager, bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	m_MaterialBuffer->UploadData(m_Handles.allMaterials.data(), bufferSize, 0);
}

void MaterialManager::CreateMaterialDescriptor()
{
	if (m_Handles.allMaterials.size() == 0)
	{
		return;
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = m_MaterialBuffer->GetHandles().buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = m_MaterialBuffer->GetHandles().bufferSize;

	BufferDescriptorUpdateInfo bufferUpdateInfo{};
	bufferUpdateInfo.binding = 0;
	bufferUpdateInfo.firstArrayElement = 0;
	bufferUpdateInfo.bufferInfos = { bufferInfo };

	BindingElementInfo bindingElementInfo{};  
	bindingElementInfo.binding = 0;
	bindingElementInfo.descriptorCount = 1;
	bindingElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingElementInfo.bufferDescriptorUpdateInfoCount = 1;
	bindingElementInfo.pBufferDescriptorUpdates = &bufferUpdateInfo;
	bindingElementInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	m_Handles.descriptor = new VulkanDescriptor(m_VulkanHandles, { bindingElementInfo }, 2);
}
