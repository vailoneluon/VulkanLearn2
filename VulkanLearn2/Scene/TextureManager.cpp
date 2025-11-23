#include "pch.h"
#include "TextureManager.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanDescriptor.h"
#include "Core/VulkanBuffer.h"

TextureManager::TextureManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, const VkSampler& sampler):
	m_VulkanHandles(vulkanHandles), 
	m_CommandManager(commandManager), 
	m_Sampler(sampler)
{
	// Load Các Default Image.
	m_DefaultDiffuseIndex = LoadTextureImage("Resources/DefaultTextures/default_diffuse.png", VK_FORMAT_R8G8B8A8_SRGB);
	m_DefaultNormalIndex = LoadTextureImage("Resources/DefaultTextures/default_normal.png", VK_FORMAT_R8G8B8A8_UNORM);
	m_DefaultSpecularIndex = LoadTextureImage("Resources/DefaultTextures/default_specular.png", VK_FORMAT_R8G8B8A8_UNORM);
	m_DefaultRoughnessIndex = LoadTextureImage("Resources/DefaultTextures/default_roughness.png", VK_FORMAT_R8G8B8A8_UNORM);
	m_DefaultMetallicIndex = LoadTextureImage("Resources/DefaultTextures/default_metallic.png", VK_FORMAT_R8G8B8A8_UNORM);
	m_DefaultOcclusionIndex = LoadTextureImage("Resources/DefaultTextures/default_occlusion.png", VK_FORMAT_R8G8B8A8_UNORM);
}

TextureManager::~TextureManager()
{
	// LƯU Ý: `handles.textureImageDescriptor` được cấp phát động trong `CreateTextureImageDescriptor`
	// nhưng không được giải phóng ở đây, có thể gây rò rỉ bộ nhớ.
	for (auto& textureImage : m_Handles.allTextureImageLoaded)
	{
		delete(textureImage);
	}
}

uint32_t TextureManager::LoadTextureImage(const std::string& imageFilePath, VkFormat imageFormat)
{
	// Kiểm tra xem texture đã được yêu cầu tải trước đó chưa bằng cách tìm trong map. 
	if (m_Handles.filePathList.find(imageFilePath) == m_Handles.filePathList.end())
	{
		// Nếu chưa, tạo một đối tượng TextureImage mới và lưu vào map.
		m_Handles.filePathList[imageFilePath] = CreateNewTextureImage(imageFilePath, imageFormat);
	}

	// Trả về ID của texture đã có hoặc vừa được tạo.
	return m_Handles.filePathList[imageFilePath]->id;
}

void TextureManager::FinalizeSetup()
{
	// Chỉ thực hiện khi có texture đã được yêu cầu.
	if (m_Handles.allTextureImageLoaded.empty()) {
		return;
	}

	UploadDataToTextureImage();
	CreateTextureImageDescriptor();
}

void TextureManager::UploadDataToTextureImage()
{
	std::vector<VulkanBuffer*> stagingBuffers;
	stagingBuffers.reserve(m_Handles.allTextureImageLoaded.size());

	VkCommandBuffer singleTimeCmd = m_CommandManager->BeginSingleTimeCmdBuffer();
	
	// Với mỗi texture, gọi hàm để tải dữ liệu lên GPU và thu thập các staging buffer.
	for (auto& textureImage : m_Handles.allTextureImageLoaded)
	{
		stagingBuffers.push_back(textureImage->textureImage->UploadTextureData(m_CommandManager, singleTimeCmd));
	}

	// Kết thúc và submit command buffer, đợi cho đến khi GPU thực thi xong.
	m_CommandManager->EndSingleTimeCmdBuffer(singleTimeCmd);

	// Sau khi GPU đã copy xong, các staging buffer không còn cần thiết và cần được giải phóng.
	for (auto& stagingBuffer : stagingBuffers)
	{
		delete(stagingBuffer);
	}
}

void TextureManager::CreateTextureImageDescriptor()
{
	BindingElementInfo textureImageElementInfo;
	textureImageElementInfo.binding = 0;
	
	// LƯU Ý: Descriptor count được set cứng là 256 thay vì số lượng texture thực tế.
	// Điều này tạo ra một mảng descriptor với kích thước cố định, cho phép shader truy cập
	// texture bằng một index (kiểu bindless). Nó hiệu quả hơn việc re-bind descriptor set
	// nhưng giới hạn tổng số texture là 256.
	//textureImageElementInfo.descriptorCount = m_Handles.allTextureImageLoaded.size();
	textureImageElementInfo.descriptorCount = DESCRIPTOR_COUNT;
	textureImageElementInfo.pImmutableSamplers = nullptr;
	textureImageElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureImageElementInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	textureImageElementInfo.useBindless = true;

	// Tạo Descriptor
	std::vector<VkDescriptorImageInfo> descImageInfos;
	descImageInfos.reserve(DESCRIPTOR_COUNT);

	// Chuẩn bị một mảng các VkDescriptorImageInfo, mỗi cái trỏ đến một image view.
	for (const auto& textureImage : m_Handles.allTextureImageLoaded)
	{
		VkDescriptorImageInfo descImageInfo{};
		descImageInfo.sampler = m_Sampler;
		descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descImageInfo.imageView = textureImage->textureImage->GetHandles().imageView;

		descImageInfos.push_back(descImageInfo);
	}

	ImageDescriptorUpdateInfo updateInfo{};
	updateInfo.binding = 0;
	updateInfo.firstArrayElement = 0;
	updateInfo.imageInfos = descImageInfos;

	textureImageElementInfo.imageDescriptorUpdateInfoCount = 1;
	textureImageElementInfo.pImageDescriptorUpdates = &updateInfo;

	std::vector<BindingElementInfo> textureBindings{ textureImageElementInfo };

	m_Handles.textureImageDescriptor = new VulkanDescriptor(m_VulkanHandles, textureBindings, 0);
}

TextureImage* TextureManager::CreateNewTextureImage(const std::string& imageFilePath, VkFormat imageFormat)
{
	// LƯU Ý: Tham số cuối cùng là `false`, nghĩa là mipmap không được tạo.
	// Đối với texture, điều này thường không tối ưu và có thể gây ra aliasing (hiện tượng răng cưa).
	// Nên cân nhắc đổi thành `true`.
	VulkanImage* image = new VulkanImage(m_VulkanHandles, imageFilePath.c_str(), imageFormat, true);
	
	TextureImage* textureImage = new TextureImage();
	textureImage->textureImage = image;
	textureImage->id = static_cast<uint32_t>(m_Handles.allTextureImageLoaded.size()); // ID chính là index trong mảng.
	std::cout << imageFilePath << std::endl;
	m_Handles.allTextureImageLoaded.push_back(textureImage);

	return textureImage;
}

TextureImage::~TextureImage()
{
	delete(textureImage);
}