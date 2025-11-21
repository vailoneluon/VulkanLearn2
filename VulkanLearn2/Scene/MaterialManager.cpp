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
	// Khởi tạo cấu trúc dữ liệu cho vật liệu mới.
	MaterialData material{};

	// --- Xử lý Diffuse Map ---
	// Kiểm tra nếu đường dẫn file diffuse map rỗng (nghĩa là model không cung cấp diffuse map).
	if (materialRawData.diffuseMapFileName == "")
	{
		// Gán trực tiếp chỉ số của diffuse map mặc định.
		material.diffuseMapIndex = m_TextureManager->m_DefaultDiffuseIndex;
	}
	else
	{
		// Nếu có đường dẫn, thử tải diffuse map.
		try 
		{
			material.diffuseMapIndex = m_TextureManager->LoadTextureImage(materialRawData.diffuseMapFileName);
		}
		catch (const std::runtime_error& e)
		{
			// Nếu quá trình tải diffuse map gặp lỗi (ví dụ: file không tồn tại hoặc hỏng),
			// ghi lại lỗi và gán chỉ số của diffuse map mặc định để chương trình tiếp tục.
			Log::Warning(e.what());
			material.diffuseMapIndex = m_TextureManager->m_DefaultDiffuseIndex;
		}
	}

	// --- Xử lý Normal Map ---
	// Kiểm tra nếu đường dẫn file normal map rỗng.
	if (materialRawData.normalMapFileName == "")
	{
		// Gán trực tiếp chỉ số của normal map mặc định.
		// LƯU Ý: Trong logic hiện tại, nếu đường dẫn rỗng, hàm LoadTextureImage vẫn được gọi với chuỗi rỗng.
		// Hàm LoadTextureImage sẽ xử lý chuỗi rỗng và trả về chỉ số mặc định.
		material.normalMapIndex = m_TextureManager->m_DefaultNormalIndex;
	}
	else
	{
		// Nếu có đường dẫn, thử tải normal map.
		try
		{
			material.normalMapIndex = m_TextureManager->LoadTextureImage(materialRawData.normalMapFileName);
		}
		catch (const std::runtime_error& e)
		{
			// Nếu quá trình tải normal map gặp lỗi, ghi lại lỗi và gán chỉ số của normal map mặc định.
			Log::Warning(e.what());
			material.normalMapIndex = m_TextureManager->m_DefaultNormalIndex;
		}
	}

	// --- Xử lý Specular Map ---
	// Kiểm tra nếu đường dẫn file specular map rỗng.
	if (materialRawData.specularMapFileName == "")
	{
		// Gán trực tiếp chỉ số của specular map mặc định.
		material.specularMapIndex = m_TextureManager->m_DefaultSpecularIndex;
	}
	else
	{
		// Nếu có đường dẫn, thử tải specular map.
		try
		{
			material.specularMapIndex = m_TextureManager->LoadTextureImage(materialRawData.specularMapFileName);

		}
		catch (const std::runtime_error& e)
		{
			// Nếu quá trình tải specular map gặp lỗi, ghi lại lỗi và gán chỉ số của specular map mặc định.
			Log::Warning(e.what());
			material.specularMapIndex = m_TextureManager->m_DefaultSpecularIndex;
		}
	}

	// --- Xử lý Roughness Map (PBR) ---
	if (materialRawData.roughnessMapFileName == "")
	{
		material.roughnessMapIndex = m_TextureManager->m_DefaultRoughnessIndex; // TODO: Implement m_DefaultRoughnessIndex in TextureManager
	}
	else
	{
		try
		{
			material.roughnessMapIndex = m_TextureManager->LoadTextureImage(materialRawData.roughnessMapFileName);
		}
		catch (const std::runtime_error& e)
		{
			Log::Warning(e.what());
			material.roughnessMapIndex = m_TextureManager->m_DefaultRoughnessIndex;
		}
	}

	// --- Xử lý Metallic Map (PBR) ---
	if (materialRawData.metallicMapFileName == "")
	{
		material.metallicMapIndex = m_TextureManager->m_DefaultMetallicIndex; // TODO: Implement m_DefaultMetallicIndex in TextureManager
	}
	else
	{
		try
		{
			material.metallicMapIndex = m_TextureManager->LoadTextureImage(materialRawData.metallicMapFileName);
		}
		catch (const std::runtime_error& e)
		{
			Log::Warning(e.what());
			material.metallicMapIndex = m_TextureManager->m_DefaultMetallicIndex;
		}
	}

	// --- Xử lý Occlusion Map (PBR) ---
	if (materialRawData.occulusionMapFileName == "") // Corrected typo: occulusion -> occlusion
	{
		material.occlusionMapIndex = m_TextureManager->m_DefaultOcclusionIndex; // TODO: Implement m_DefaultOcclusionIndex in TextureManager
	}
	else
	{
		try
		{
			material.occlusionMapIndex = m_TextureManager->LoadTextureImage(materialRawData.occulusionMapFileName); // Corrected typo: occulusion -> occlusion
		}
		catch (const std::runtime_error& e)
		{
			Log::Warning(e.what());
			material.occlusionMapIndex = m_TextureManager->m_DefaultOcclusionIndex;
		}
	}

	// Thêm vật liệu đã cấu hình vào danh sách tất cả các vật liệu được quản lý.
	m_Handles.allMaterials.push_back(material);
	// Trả về chỉ số của vật liệu vừa được thêm vào.
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
