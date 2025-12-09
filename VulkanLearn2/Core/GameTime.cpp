#include "pch.h"
#include "GameTime.h"

// =================================================================================================
// KHỞI TẠO CÁC BIẾN STATIC
// =================================================================================================

Core::Time::Clock::time_point Core::Time::s_StartTime;
Core::Time::Clock::time_point Core::Time::s_LastFrameTime;

float Core::Time::s_DeltaTime = 0.0f;
float Core::Time::s_TimeSinceStart = 0.0f;

int Core::Time::s_CurrentDeltaTimeIndex = 0;

// Khởi tạo mảng lịch sử với giá trị 0.
std::array<float, Core::Time::s_HistorySize> Core::Time::s_DeltaTimeHistory = {};


// =================================================================================================
// IMPLEMENTATION
// =================================================================================================

void Core::Time::Init()
{
	// Lấy mốc thời gian hiện tại.
	s_StartTime = Clock::now();
	s_LastFrameTime = s_StartTime;

	// Khởi tạo bộ đệm lịch sử với giá trị mặc định là 60 FPS (0.01666s).
	// Điều này giúp tránh tình trạng "nhảy hình" (hitch) ở frame đầu tiên khi chưa có dữ liệu lịch sử.
	s_DeltaTimeHistory.fill(0.01666f);
}

void Core::Time::Update()
{
	auto now = Clock::now();

	// 1. Tính tổng thời gian chạy từ lúc Start.
	//s_TimeSinceStart = std::chrono::duration<float>(now - s_StartTime).count();
	
	// 2. Tính Raw Delta Time (Thời gian thực tế trôi qua từ frame trước).
	float rawDeltaTime = std::chrono::duration<float>(now - s_LastFrameTime).count();

	// 3. Kẹp giá trị (Clamping):
	// Giới hạn DeltaTime tối đa là 0.1s (tương đương 10 FPS).
	// Mục đích: Ngăn chặn các lỗi vật lý (xuyên tường) hoặc logic game bị văng quá xa
	// khi máy bị treo tạm thời (lag spike) hoặc khi đang debug (breakpoint).
	const float MAX_DELTA_TIME = 0.1f;
	if (rawDeltaTime > MAX_DELTA_TIME)
	{
		rawDeltaTime = MAX_DELTA_TIME;
	}

	// 4. Cập nhật Bộ đệm vòng (Ring Buffer):
	// Ghi đè giá trị mới vào vị trí index hiện tại trong lịch sử.
	s_DeltaTimeHistory[s_CurrentDeltaTimeIndex] = rawDeltaTime;
	s_CurrentDeltaTimeIndex = (s_CurrentDeltaTimeIndex + 1) % s_HistorySize;

	// 5. Tính toán Trung bình động (Simple Moving Average - SMA):
	// Cộng tổng các giá trị trong lịch sử và chia trung bình.
	// Việc này giúp loại bỏ nhiễu (jitter) do hệ điều hành gây ra, giúp chuyển động mượt mà hơn.
	float dtSum = 0;
	for (float dt : s_DeltaTimeHistory)
	{
		dtSum += dt;
	}

	s_DeltaTime = dtSum / (float)s_HistorySize;

	// 6. Cập nhật mốc thời gian cho frame tiếp theo.
	s_LastFrameTime = now;
}