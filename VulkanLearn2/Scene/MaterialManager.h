#pragma once
#include "Core\VulkanContext.h"
#include "Core\VulkanBuffer.h"

class VulkanCommandManager;
class TextureManager;
class VulkanDescriptor;

// =================================================================================================
// Struct: MaterialData
// Mô tả: Dữ liệu vật liệu được gửi lên GPU (std140/std430 layout).
// =================================================================================================
struct MaterialData
{
	// Index trỏ vào mảng texture (TextureManager)
	alignas(4) uint32_t diffuseMapIndex;
	alignas(4) uint32_t normalMapIndex;
	alignas(4) uint32_t specularMapIndex;
	alignas(4) uint32_t roughnessMapIndex;
	alignas(4) uint32_t metallicMapIndex;
	alignas(4) uint32_t occlusionMapIndex;

	alignas(4) uint32_t _padding;
	alignas(4) uint32_t _padding2;
};

// =================================================================================================
// Struct: MaterialRawData
// Mô tả: Dữ liệu thô của vật liệu (đường dẫn file texture) trước khi load.
// =================================================================================================
struct MaterialRawData
{
	std::string diffuseMapFileName = "";
	std::string normalMapFileName = "";
	std::string specularMapFileName = "";
	std::string roughnessMapFileName = "";
	std::string metallicMapFileName = "";
	std::string occulusionMapFileName = "";
};

// =================================================================================================
// Struct: MaterialManagerHandles
// Mô tả: Struct chứa các handle và dữ liệu nội bộ của MaterialManager.
// =================================================================================================
struct MaterialManagerHandles
{
	std::vector<MaterialData> allMaterials;
	VulkanDescriptor* descriptor;
};

// =================================================================================================
// Class: MaterialManager
// Mô tả: 
//      Quản lý việc tạo, lưu trữ và descriptor cho các vật liệu trong scene.
//      Tương tác với TextureManager để load các texture cần thiết.
// =================================================================================================
class MaterialManager
{
public:
	// Constructor: Khởi tạo MaterialManager.
	MaterialManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, TextureManager* textureManager);
	~MaterialManager();

	// Getter: Lấy các handle nội bộ.
	const MaterialManagerHandles& GetHandles() const { return m_Handles; }

	// Load một vật liệu từ dữ liệu thô.
	// Trả về index của vật liệu trong buffer.
	uint32_t LoadMaterial(const MaterialRawData& materialRawData);

	// Getter: Lấy descriptor set chứa buffer vật liệu.
	VulkanDescriptor* GetDescriptor();
	
	// Hoàn tất quá trình thiết lập: tạo buffer và descriptor cho tất cả vật liệu.
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
