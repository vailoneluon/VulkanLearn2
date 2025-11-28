#pragma once
#include "Scene.h"
#include "Component.h"

class TransformSystem
{
public:
	static void UpdateTransformMatrix(Scene* scene)
	{
		auto view = scene->GetRegistry().view<TransformComponent>();

		view.each([](auto e, const TransformComponent& transform)
			{
				if (transform.m_IsDirty)
				{
					UpdateTransformMatrix(transform);
					transform.m_IsDirty = false;
				}
			});
	}

private:
	static void UpdateTransformMatrix(const TransformComponent& transform)
	{
		glm::mat4 mat = glm::mat4(1.0f);
		mat = glm::translate(mat, transform.m_Position);
		mat = glm::rotate(mat, glm::radians(transform.m_Rotation.z), { 0,0,1 });
		mat = glm::rotate(mat, glm::radians(transform.m_Rotation.y), { 0,1,0 });
		mat = glm::rotate(mat, glm::radians(transform.m_Rotation.x), { 1,0,0 });
		mat = glm::scale(mat, transform.m_Scale);

		transform.m_TransformMatrix = mat;
	}
};