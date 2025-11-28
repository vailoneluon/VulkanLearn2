#include "pch.h"
#include "LightManager.h"
#include "Core/VulkanBuffer.h"
#include "Core/VulkanDescriptor.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanImage.h"
#include "Scene/Scene.h"
#include "Component.h"

LightManager::LightManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager, Scene* scene, const VulkanSampler* sampler, uint32_t maxFramesInFlight):
	m_VulkanHandles(vulkanHandles),
	m_CommandManager(commandManager),
	m_Scene(scene),
	m_VulkanSampler(sampler),
	m_MaxFramesInFlight(maxFramesInFlight)
{
	auto view = m_Scene->GetRegistry().view<LightComponent, TransformComponent>();

	view.each([&](auto e, const LightComponent& lightComponent, const TransformComponent& transformComponent) 
		{
			if (lightComponent.IsEnable == false) return;
			m_AllSceneGpuLights.push_back(lightComponent.Data.ToGPU(transformComponent.GetPosition()));
		});

	CreateDummyShadowMap();
	CreateShadowMappingTexture(maxFramesInFlight);
	CreateLightBuffers(maxFramesInFlight);
	CreateDescriptors(maxFramesInFlight);

	// Cập nhật dữ liệu lần đầu.
	UpdateLightSpaceMatrices();
	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		UploadLightData(i);
	}
}

LightManager::~LightManager()
{
	for (const auto& buffer : m_SceneLightBuffers)
	{
		delete(buffer);
	}

	for (size_t i = 0; i < m_ShadowMappingImages.size(); i++)
	{
		for (const auto& image : m_ShadowMappingImages[i])
		{
			delete(image);
		}
	}

	delete(m_DummyShadowMap); // Giải phóng dummy shadow map
}

void LightManager::CreateLightBuffers(uint32_t maxFramesInFlight)
{
	// Nếu không có đèn nào, không cần tạo buffer.
	if (m_AllSceneGpuLights.empty())
	{
		m_SceneLightBuffers.resize(maxFramesInFlight, nullptr); // Đảm bảo vector có kích thước đúng nhưng chứa null
		return;
	}

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
	}
}

void LightManager::CreateDescriptors(uint32_t maxFramesInFlight)
{
	m_LightBufferDescriptors.resize(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		// Binding 0 cho Light Buffer chưa dữ liệu ánh sáng.
		// Nếu không có đèn, sử dụng một buffer rỗng hoặc một buffer dummy để tránh crash.
		VkDescriptorBufferInfo bufferInfo{};
		if (m_SceneLightBuffers[i]) // Chỉ truy cập nếu buffer tồn tại
		{
			bufferInfo.buffer = m_SceneLightBuffers[i]->GetHandles().buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = m_SceneLightBuffers[i]->GetHandles().bufferSize;
		}
		else
		{
			// Xử lý trường hợp không có đèn: có thể tạo một buffer dummy nhỏ hoặc
			// đảm bảo shader xử lý mảng đèn rỗng. Hiện tại, để đơn giản, giả định
			// shader sẽ không access nếu m_AllSceneGpuLights rỗng.
			// Nếu không có đèn, range = 0, điều này hợp lệ cho shader SSBO.
			bufferInfo.buffer = VK_NULL_HANDLE; // Hoặc một buffer dummy hợp lệ 
			bufferInfo.offset = 0;
			bufferInfo.range = 0; 
		}

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

		// Binding 1 Cho Shadow Image chứa dữ liệu bóng của từng nguồn sáng.
		std::vector<VkDescriptorImageInfo> imageInfos{};
		
		// Lấp đầy imageInfos với shadow maps thực tế
		for (size_t j = 0; j < m_ShadowMappingImages[i].size(); j++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_ShadowMappingImages[i][j]->GetHandles().imageView;
			imageInfo.sampler = m_VulkanSampler->getShadowSampler();
			imageInfos.push_back(imageInfo);
		}

		if (imageInfos.empty())
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_DummyShadowMap->GetHandles().imageView;
			imageInfo.sampler = m_VulkanSampler->getShadowSampler();
			imageInfos.push_back(imageInfo);
		}

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 1;
		imageUpdateInfo.firstArrayElement = 0;
		imageUpdateInfo.imageInfos = imageInfos;

		BindingElementInfo elementInfo2{};
		elementInfo2.binding = 1;
		elementInfo2.descriptorCount = MAX_SHADOW_DESCRIPTOR;
		elementInfo2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		elementInfo2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		elementInfo2.imageDescriptorUpdateInfoCount = 1;
		elementInfo2.pImageDescriptorUpdates = &imageUpdateInfo;
		elementInfo2.useBindless = true; // Bật cờ bindless cho mảng shadow map

		m_LightBufferDescriptors[i] = new VulkanDescriptor(m_VulkanHandles, {elementInfo, elementInfo2}, 1);
	}
}

void LightManager::UploadLightData(uint32_t currentFrame)
{
	VkDeviceSize bufferSize = sizeof(GPULight) * m_AllSceneGpuLights.size();
	m_SceneLightBuffers[currentFrame]->UploadData(m_AllSceneGpuLights.data(), bufferSize, 0);
}

void LightManager::CreateShadowMappingTexture(uint32_t maxFramesInFlight)
{
	m_ShadowMappingImages.resize(maxFramesInFlight);
	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		for (auto& light : m_AllSceneGpuLights)
		{
			if (light.params.z != -1) // Chỉ tạo shadow map cho những đèn có shadow map index hợp lệ
			{
				VulkanImageCreateInfo imageInfo{};
				imageInfo.format = VK_FORMAT_D32_SFLOAT;
				imageInfo.width = SHADOW_SIZE;
				imageInfo.height = SHADOW_SIZE;
				imageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageInfo.mipLevels = 1;

				VulkanImageViewCreateInfo imageViewInfo{};
				imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
				imageViewInfo.format = VK_FORMAT_D32_SFLOAT;
				imageViewInfo.mipLevels = 1;

				VulkanImage* shadowImage = new VulkanImage(m_VulkanHandles, imageInfo, imageViewInfo);

				light.params.z = static_cast<float>(m_ShadowMappingImages[i].size());
				m_ShadowMappingImages[i].push_back(shadowImage);
			}
		}
	}
}

void LightManager::CreateDummyShadowMap()
{
	VulkanImageCreateInfo imageInfo{};
	imageInfo.format = VK_FORMAT_D32_SFLOAT;
	imageInfo.width = 1;
	imageInfo.height = 1;
	imageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.mipLevels = 1;

	VulkanImageViewCreateInfo imageViewInfo{};
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewInfo.format = VK_FORMAT_D32_SFLOAT;
	imageViewInfo.mipLevels = 1;

	m_DummyShadowMap = new VulkanImage(m_VulkanHandles, imageInfo, imageViewInfo);
}

void LightManager::UpdateLightSpaceMatrices()
{
	// Tạm thời chỉ xử lý cho đèn directional và spot.
	// Trong thực tế, bạn sẽ cần tính toán frustum của camera để tối ưu hóa shadow map cho đèn directional.
	float nearPlane = 0.1f; // Khoảng nhìn gần của đèn
	float farPlane = 100.0f; // Khoảng nhìn xa của đèn
	float orthoSize = 5.0f; // Kích thước của frustum orthographic

	for (auto& light : m_AllSceneGpuLights)
	{
		// Chỉ tính cho đèn có shadow map
		if (light.params.z < 0)
		{
			continue;
		}

		glm::mat4 lightProjection;
		glm::mat4 lightView;

		LightType type = static_cast<LightType>(static_cast<int>(light.position.w));

		if (type == LightType::Directional)
		{
			// Ma trận chiếu Orthographic cho đèn directional
			// (left, right, bottom, top, zNear, zFar)
			lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
			// FIX LỖI NGƯỢC TRỤC Y CỦA VULKAN
			lightProjection[1][1] *= -1; 

			// Ma trận View nhìn từ vị trí của đèn
			glm::vec3 lightDir = glm::normalize(glm::vec3(light.direction));
			glm::vec3 target = glm::vec3(0.0f); // Target là gốc tọa độ (có thể là vị trí người chơi sau này)
			glm::vec3 eyePos = target - lightDir * 40.0f; // Lùi lại 40m để bao quát

			lightView = glm::lookAt(eyePos, target, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else if (type == LightType::Spot)
		{
			// Ma trận chiếu Perspective cho đèn spot
			// FOV được tính từ outer cutoff. light.params.y đã là cos(outerCutoff)
			float fov = glm::degrees(glm::acos(light.params.y)) * 2.0f;
			// light.direction.w chứa range của đèn, dùng làm farPlane cho perspective
			lightProjection = glm::perspective(glm::radians(fov), 1.0f, nearPlane, light.direction.w); // Aspect ratio 1.0 cho shadow map vuông
			// FIX LỖI NGƯỢC TRỤC Y CỦA VULKAN
			lightProjection[1][1] *= -1; 

			// Ma trận View nhìn từ vị trí của đèn spot
			glm::vec3 lightPos = glm::vec3(light.position);
			glm::vec3 lightDir = glm::normalize(glm::vec3(light.direction));
			lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0, 1.0, 0.0));
		}
		else
		{
			// Bỏ qua Point Light cho shadow map đơn giản (cần cubemap shadow)
			continue;
		}

		light.lightSpaceMatrix = lightProjection * lightView;
	}
}
