#pragma once
#include "Core/VulkanContext.h"

class VulkanCommandManager;
class VulkanImage;
class VulkanDescriptor;

struct TextureImage
{
	uint32_t id;
	VulkanImage* textureImage;

	~TextureImage();
};

struct TextureManagerHandles
{
	std::vector<TextureImage*> allTextureImageLoaded;
	std::unordered_map<std::string, TextureImage*> filePathList;
	
	VulkanDescriptor* textureImageDescriptor;
};

class TextureManager
{
public:
	TextureManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, const VkSampler& sampler);
	~TextureManager();

	VulkanDescriptor* getDescriptor() const { return handles.textureImageDescriptor; };

	uint32_t LoadTextureImage(const std::string& imageFilePath);
	void CopyDataToTextureImage();
	void CreateTextureImageDescriptor();
	void UpdateTextureImageDescriptorBinding();
private:
	const VulkanHandles& vk;
	const VkSampler& spl;
	VulkanCommandManager* cmd;

	TextureManagerHandles handles;

	TextureImage* CreateNewTextureImage(const std::string& imageFilePath);
};
