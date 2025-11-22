#pragma once
#include "Core/VulkanContext.h"
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
class VulkanCommandManager;
class VulkanImage;
class VulkanDescriptor;


// Struct chứa thông tin về một texture đã được quản lý bởi Manager.
struct TextureImage
{
	uint32_t id;
	VulkanImage* textureImage;

	~TextureImage();
};

// Struct chứa tất cả các handle và dữ liệu nội bộ của TextureManager.
struct TextureManagerHandles
{
	std::vector<TextureImage*> allTextureImageLoaded;
	std::unordered_map<std::string, TextureImage*> filePathList;
	
	VulkanDescriptor* textureImageDescriptor;
};

// Class quản lý việc tải, lưu trữ và truy cập tất cả các texture trong scene.
// Đảm bảo mỗi file ảnh chỉ được tải lên GPU một lần.
class TextureManager
{
public:
	TextureManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, const VkSampler& sampler);
	~TextureManager();

	// Lấy ra descriptor set chứa mảng các texture.
	VulkanDescriptor* getDescriptor() const { return m_Handles.textureImageDescriptor; };

	// --- Texture ID của các texture mặc định sử dụng khi mesh không có hoặc load lỗi.
	uint32_t m_DefaultDiffuseIndex;
	uint32_t m_DefaultNormalIndex;
	uint32_t m_DefaultSpecularIndex;
	uint32_t m_DefaultRoughnessIndex;
	uint32_t m_DefaultMetallicIndex;
	uint32_t m_DefaultOcclusionIndex;

	// Yêu cầu tải một texture từ đường dẫn file.
	// Trả về ID của texture, có thể dùng trong shader.
	uint32_t LoadTextureImage(const std::string& imageFilePath, VkFormat imageFormat);

	// Hoàn tất quá trình thiết lập: tải tất cả texture đã được yêu cầu lên GPU và tạo descriptor.
	void FinalizeSetup();

private:
	// --- Tham chiếu đến các đối tượng Vulkan bên ngoài ---
	const VulkanHandles& m_VulkanHandles;
	const VkSampler& m_Sampler;
	VulkanCommandManager* m_CommandManager;

	// --- Dữ liệu nội bộ ---
	TextureManagerHandles m_Handles;

	

	// Hiện tại chưa sử dụng Bindless Descriptor nên cần 1 số lượng để khởi tạo
	const uint32_t DESCRIPTOR_COUNT = 256;

	// --- Hàm helper private ---
	TextureImage* CreateNewTextureImage(const std::string& imageFilePath, VkFormat imageFormat);
	void UploadDataToTextureImage();
	void CreateTextureImageDescriptor();
};