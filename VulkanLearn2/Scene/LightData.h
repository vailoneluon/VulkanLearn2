#pragma once
#include <glm/glm.hpp>

// =================================================================================================
// Enum: LightType
// Mô tả: Các loại nguồn sáng được hỗ trợ.
// =================================================================================================
enum class LightType : int
{
	Directional = 0,
	Point = 1,
	Spot = 2
};

// =================================================================================================
// Struct: GPULight
// Mô tả: Cấu trúc dữ liệu ánh sáng được gửi lên GPU (SSBO).
//        Dữ liệu được pack chặt chẽ theo alignment của vec4 (16 bytes) để tối ưu hóa bộ nhớ.
// =================================================================================================
struct GPULight
{
	glm::vec4 position;   // xyz: Pos, w: Type
	glm::vec4 direction;  // xyz: Dir, w: Range
	glm::vec4 color;      // rgb: Color, w: Intensity
	glm::vec4 params;     // x: Inner, y: Outer, z: ShadowIdx, w: Radius
	alignas(16) glm::mat4 lightSpaceMatrix; // Ma trận để chuyển từ world-space sang light-space
};

// =================================================================================================
// Struct: Light
// Mô tả: Cấu trúc dữ liệu ánh sáng trên CPU.
//        Dễ đọc, dễ sử dụng và cung cấp các hàm helper (Builder pattern) để tạo đèn.
// =================================================================================================
struct Light
{
	// --- Định danh ---
	LightType type;

	// --- Thông số cơ bản ---
	glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
	glm::vec3 color{ 1.0f }; // Màu trắng

	float intensity = 1.0f;  // Cường độ
	float range = 10.0f;     // Tầm xa (Point/Spot)

	// --- Thông số Spot Light ---
	float innerCutoff = 12.5f; // Độ (Degrees)
	float outerCutoff = 17.5f; // Độ (Degrees)

	// --- Thông số PBR / Shadow ---
	float sourceRadius = 0.0f; // Kích thước bóng đèn (cho PBR Specular)
	
	// Mặc định là -1 nếu hasShadow = false
	// Khởi tạo là 0 nếu hasShadow = true
	// Trong LightManager tính toán để lưu shadowMapIndex thành id của ShadowMap trong shader.
	int shadowMapIndex = -1;   // -1 là không có bóng đổ
	glm::mat4 m_LightSpaceMatrix{ 1.0f }; // Ma trận không gian ánh sáng (View * Projection từ góc nhìn của đèn)


	// =========================================================
	// HÀM CHUYỂN ĐỔI (QUAN TRỌNG NHẤT)
	// =========================================================
	
	// Chuyển đổi dữ liệu từ CPU struct sang GPU struct.
	GPULight ToGPU(glm::vec3 position) const
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

		// Row 4-7: Light Space Matrix
		gpu.lightSpaceMatrix = m_LightSpaceMatrix;

		return gpu;
	}

	// =========================================================
	// HELPER FUNCTIONS (BUILDER) - Giúp khởi tạo nhanh
	// =========================================================

	static Light CreateDirectional(const glm::vec3& dir, const glm::vec3& col, float intensity, bool hasShadow = false)
	{
		Light l;
		l.type = LightType::Directional;
		l.direction = dir;
		l.color = col;
		l.intensity = intensity;

		if (hasShadow)	l.shadowMapIndex = 0;

		return l;
	}

	static Light CreatePoint(const glm::vec3& col, float intensity, float range, float radius = 0.05f, bool hasShadow = false)
	{
		Light l;
		l.type = LightType::Point;
		l.color = col;
		l.intensity = intensity;
		l.range = range;
		l.sourceRadius = radius;

		if (hasShadow)	l.shadowMapIndex = 0;

		return l;
	}

	static Light CreateSpot(const glm::vec3& dir, const glm::vec3& col, float intensity, float range, float innerDeg, float outerDeg, bool hasShadow = false)
	{
		Light l;
		l.type = LightType::Spot;
		l.direction = dir;
		l.color = col;
		l.intensity = intensity;
		l.range = range;
		l.innerCutoff = innerDeg;
		l.outerCutoff = outerDeg;

		if (hasShadow)	l.shadowMapIndex = 0;

		return l;
	}
};