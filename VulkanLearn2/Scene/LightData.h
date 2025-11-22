#pragma once
#include <glm/glm.hpp>

// Enum cho dễ đọc code
enum class LightType : int
{
	Directional = 0,
	Point = 1,
	Spot = 2
};

// ---------------------------------------------------------
// 1. GPU LIGHT (Data thuần túy, Pack theo vec4 alignment)
// ---------------------------------------------------------
struct GPULight
{
	glm::vec4 position;   // xyz: Pos, w: Type
	glm::vec4 direction;  // xyz: Dir, w: Range
	glm::vec4 color;      // rgb: Color, w: Intensity
	glm::vec4 params;     // x: Inner, y: Outer, z: ShadowIdx, w: Radius
};

// ---------------------------------------------------------
// 2. CPU LIGHT (Tường minh, Dễ dùng, Có Builder pattern)
// ---------------------------------------------------------
struct Light
{
	// --- Định danh ---
	LightType type;

	// --- Thông số cơ bản ---
	glm::vec3 position{ 0.0f };
	glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
	glm::vec3 color{ 1.0f }; // Màu trắng

	float intensity = 1.0f;  // Cường độ
	float range = 10.0f;     // Tầm xa (Point/Spot)

	// --- Thông số Spot Light ---
	float innerCutoff = 12.5f; // Độ (Degrees)
	float outerCutoff = 17.5f; // Độ (Degrees)

	// --- Thông số PBR / Shadow ---
	float sourceRadius = 0.0f; // Kích thước bóng đèn (cho PBR Specular)
	int shadowMapIndex = -1;   // -1 là không có bóng đổ

	// =========================================================
	// HÀM CHUYỂN ĐỔI (QUAN TRỌNG NHẤT)
	// =========================================================
	GPULight ToGPU() const
	{
		GPULight gpu{};

		// Row 0: Position & Type
		gpu.position = glm::vec4(position, static_cast<float>(type));

		// Row 1: Direction & Range
		// Lưu ý: Direction nên được normalize để shader đỡ phải tính
		gpu.direction = glm::vec4(glm::normalize(direction), range);

		// Row 2: Color & Intensity
		gpu.color = glm::vec4(color, intensity);

		// Row 3: Params
		// Chuyển đổi độ sang cos(rad) ngay tại đây để Shader chỉ việc dùng
		float innerCos = glm::cos(glm::radians(innerCutoff));
		float outerCos = glm::cos(glm::radians(outerCutoff));

		// Nếu không phải Spot light, mấy cái cutoff này không quan trọng nhưng cứ gán cho an toàn
		if (type != LightType::Spot)
		{
			innerCos = 0.0f;
			outerCos = 0.0f;
		}

		gpu.params = glm::vec4(innerCos, outerCos, (float)shadowMapIndex, sourceRadius);

		return gpu;
	}

	// =========================================================
	// HELPER FUNCTIONS (BUILDER) - Giúp khởi tạo nhanh
	// =========================================================

	static Light CreateDirectional(const glm::vec3& dir, const glm::vec3& col, float intensity)
	{
		Light l;
		l.type = LightType::Directional;
		l.direction = dir;
		l.color = col;
		l.intensity = intensity;
		return l;
	}

	static Light CreatePoint(const glm::vec3& pos, const glm::vec3& col, float intensity, float range, float radius = 0.05f)
	{
		Light l;
		l.type = LightType::Point;
		l.position = pos;
		l.color = col;
		l.intensity = intensity;
		l.range = range;
		l.sourceRadius = radius;
		return l;
	}

	static Light CreateSpot(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& col, float intensity, float range, float innerDeg, float outerDeg)
	{
		Light l;
		l.type = LightType::Spot;
		l.position = pos;
		l.direction = dir;
		l.color = col;
		l.intensity = intensity;
		l.range = range;
		l.innerCutoff = innerDeg;
		l.outerCutoff = outerDeg;
		return l;
	}
};