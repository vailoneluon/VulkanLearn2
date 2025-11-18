#pragma once
#include "Core\VulkanContext.h"
#include "Core\VulkanBuffer.h"

class VulkanCommandManager;
class TextureManager;
class VulkanDescriptor;


struct MaterialData
{
	// Index trỏ vào mảng texture (TextureManager)
	alignas(4) uint32_t diffuseMapIndex;
	alignas(4) uint32_t normalMapIndex;
	alignas(4) uint32_t specularMapIndex;

	alignas(4) uint32_t _padding;
};

struct MaterialRawData
{
	std::string diffuseMapFileName = "";
	std::string normalMapFileName = "";
	std::string specularMapFileName = "";
};

struct MaterialManagerHandles
{
	std::vector<MaterialData> allMaterials;
	VulkanDescriptor* descriptor;
};

class MaterialManager
{
public:
	MaterialManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, TextureManager* textureManager);
	~MaterialManager();

	const MaterialManagerHandles& GetHandles() const { return m_Handles; }

	uint32_t LoadMaterial(const MaterialRawData& materialRawData);


	VulkanDescriptor* GetDescriptor();
	void Finalize();
private:
	const VulkanHandles& m_VulkanHandles;
	VulkanCommandManager* m_CommandManager;
	TextureManager* m_TextureManager;

	MaterialManagerHandles m_Handles;

	VulkanBuffer* m_MaterialBuffer;
	
	void CreateMaterialBuffer();
	void CreateMaterialDescriptor();
};
