#pragma once

// entt.hpp được kỳ vọng đã được include trong pch.h
#include <string>

/**
 * @class Scene
 * @brief Quản lý tất cả các entity và component trong một thế giới game thông qua registry.
 * 
 * Scene class đóng vai trò là container chính cho thế giới ECS (Entity-Component-System).
 * Nó đóng gói entt::registry và cung cấp các hàm tiện ích để tạo và hủy các entity.
 * Các System sẽ hoạt động trên registry được sở hữu bởi Scene này.
 */
class Scene
{
public:
	Scene();
	~Scene();

	/**
	 * @brief Tạo một entity mới trong registry của scene.
	 * @param name Tên định danh (debug name) cho entity. Nếu trống, một tên mặc định sẽ được sử dụng.
	 * @return Handle (mã định danh) của entity vừa được tạo.
	 *
	 * Mặc định, hàm này sẽ gắn một NameComponent và một TransformComponent vào entity mới.
	 */
	entt::entity CreateEntity(const std::string& name = std::string());

	/**
	 * @brief Hủy một entity và tất cả các component của nó.
	 * @param entity Handle của entity cần hủy.
	 */
	void DestroyEntity(entt::entity entity);

	/**
	 * @brief Cung cấp quyền truy cập trực tiếp vào registry bên dưới.
	 * @return Một tham chiếu đến đối tượng entt::registry.
	 *
	 * Đây là lối vào chính để các System truy vấn và thao tác với các entity và component.
	 */
	entt::registry& GetRegistry() { return m_Registry; }

private:
	entt::registry m_Registry; //!< Registry ECS cốt lõi, sở hữu tất cả các entity và component.
};

