#include "pch.h"
#include "TextureManager.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanDescriptor.h"

TextureManager::TextureManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, const VkSampler& sampler):
	vk(vulkanHandles), cmd(commandManager), spl(sampler)
{

}

TextureManager::~TextureManager()
{
	for (auto& textureImage : handles.allTextureImageLoaded)
	{
		delete(textureImage);
	}
}

uint32_t TextureManager::LoadTextureImage(const std::string& imageFilePath)
{
	if (handles.filePathList.find(imageFilePath) == handles.filePathList.end())
	{
		handles.filePathList[imageFilePath] = CreateNewTextureImage(imageFilePath);
	}

	return handles.filePathList[imageFilePath]->id;
}

void TextureManager::CreateTextureImageDescriptor()
{
	BindingElementInfo textureImageElementInfo;
	textureImageElementInfo.binding = 0;
	textureImageElementInfo.descriptorCount = handles.allTextureImageLoaded.size();
	textureImageElementInfo.pImmutableSamplers = nullptr;
	textureImageElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureImageElementInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<BindingElementInfo> textureBindings{ textureImageElementInfo };

	handles.textureImageDescriptor = new VulkanDescriptor(vk, textureBindings, 0);
}

void TextureManager::UpdateTextureImageDescriptorBinding()
{
	std::vector<VkDescriptorImageInfo> descImageInfos;
	for (const auto& textureImage : handles.allTextureImageLoaded)
	{
		VkDescriptorImageInfo descImageInfo{};
		descImageInfo.sampler = spl;
		descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descImageInfo.imageView = textureImage->textureImage->getHandles().imageView;
		
		descImageInfos.push_back(descImageInfo);
	}

	ImageBindingUpdateInfo updateInfo{};
	updateInfo.binding = 0;
	updateInfo.firstArrayElement = 0;
	updateInfo.imageInfoCount = handles.allTextureImageLoaded.size();
	updateInfo.imageInfos = descImageInfos.data();

	handles.textureImageDescriptor->UpdateImageBinding(1, &updateInfo);
}

TextureImage* TextureManager::CreateNewTextureImage(const std::string& imageFilePath)
{
	VulkanImage* image = new VulkanImage(vk, cmd, imageFilePath.c_str(), true);
	TextureImage* textureImage = new TextureImage;
	textureImage->textureImage = image;
	textureImage->id = handles.allTextureImageLoaded.size();
	handles.allTextureImageLoaded.push_back(textureImage);

	return textureImage;
}

TextureImage::~TextureImage()
{
	delete(textureImage);
}
