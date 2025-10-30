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

	// Yêu cầu tải một texture từ đường dẫn file.
	// Trả về ID của texture, có thể dùng trong shader.
	uint32_t LoadTextureImage(const std::string& imageFilePath);

	// Hoàn tất quá trình thiết lập: tải tất cả texture đã được yêu cầu lên GPU và tạo descriptor.
	void FinalizeSetup();

	// Cập nhật descriptor set với thông tin của các texture đã được tải.
	void UpdateTextureImageDescriptorBinding();

private:
	// --- Tham chiếu đến các đối tượng Vulkan bên ngoài ---
	const VulkanHandles& m_VulkanHandles;
	const VkSampler& m_Sampler;
	VulkanCommandManager* m_CommandManager;

	// --- Dữ liệu nội bộ ---
	TextureManagerHandles m_Handles;

	// --- Hàm helper private ---
	TextureImage* CreateNewTextureImage(const std::string& imageFilePath);
	void UploadDataToTextureImage();
	void CreateTextureImageDescriptor();
};