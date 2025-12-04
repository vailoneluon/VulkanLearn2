#include "pch.h"
#include "Scene.h"
#include "Component.h"

Scene::Scene()
{
	// Hàm khởi tạo Scene, hiện tại để trống.
	// Có thể được dùng để thiết lập các hệ thống (system) mặc định trong tương lai.
}

Scene::~Scene()
{
	// Hàm hủy Scene.
	// entt::registry sẽ tự động dọn dẹp tất cả các entity và component khi bị hủy.
}

entt::entity Scene::CreateEntity(const std::string& name)
{
	// Tạo một entity mới trong registry.
	entt::entity entity = m_Registry.create();
	
	// Gắn NameComponent để định danh. Rất hữu ích cho việc debug.
	// Nếu không có tên nào được cung cấp, sử dụng một tên mặc định.
	m_Registry.emplace<NameComponent>(entity, name.empty() ? "Unnamed Entity" : name);

	// Gắn TransformComponent làm mặc định.
	// Hầu hết mọi entity tồn tại trong thế giới 3D đều cần có vị trí/góc xoay/tỷ lệ.
	m_Registry.emplace<TransformComponent>(entity);

	return entity;
}

void Scene::DestroyEntity(entt::entity entity)
{
	// Hủy entity được chỉ định khỏi registry.
	// Thao tác này cũng sẽ tự động xóa tất cả các component đã được gắn vào entity đó.
	m_Registry.destroy(entity);
}
