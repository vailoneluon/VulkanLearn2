#pragma once
#include <chrono>
#include <array>

namespace Core
{
	/**
	 * @class Time
	 * @brief Hệ thống quản lý thời gian trung tâm của Engine.
	 * 
	 * Class này chịu trách nhiệm đo đạc và cung cấp DeltaTime (thời gian trôi qua giữa các frame).
	 * Nó tích hợp thuật toán "Simple Moving Average" để làm mượt DeltaTime, giúp loại bỏ hiện tượng
	 * giật cục (micro-stuttering) thường gặp khi bật VSync (60Hz), đảm bảo Camera và Animation di chuyển mượt mà.
	 */
	class Time
	{
	public:
		/**
		 * @brief Khởi tạo các mốc thời gian ban đầu.
		 * Cần được gọi một lần duy nhất khi Engine khởi động.
		 */
		static void Init();

		/**
		 * @brief Cập nhật thời gian cho frame hiện tại.
		 * Cần được gọi ở đầu mỗi vòng lặp (Game Loop), trước khi xử lý bất kỳ logic nào.
		 */
		static void Update();

		/**
		 * @brief Lấy DeltaTime đã được làm mượt (Smoothed Delta Time).
		 * Sử dụng giá trị này cho các phép tính di chuyển (Movement), Animation, và VFX.
		 * @return Thời gian trôi qua giữa frame trước và frame này (tính bằng giây).
		 */
		static float GetDeltaTime() { return s_DeltaTime; }

		/**
		 * @brief Lấy tổng thời gian từ khi game bắt đầu.
		 * @return Thời gian tính bằng giây.
		 */
		static float GetCurrentTime() { return s_TimeSinceStart; }

	private:
		using Clock = std::chrono::steady_clock;

		// --- Các mốc thời gian ---
		static Clock::time_point s_StartTime;     // Thời điểm Engine bắt đầu chạy.
		static Clock::time_point s_LastFrameTime; // Thời điểm kết thúc frame trước đó.

		// --- Dữ liệu thời gian ---
		static float s_DeltaTime;       // DeltaTime hiện tại (sau khi đã làm mượt).
		static float s_TimeSinceStart;  // Tổng thời gian chạy.

		// --- Cấu hình thuật toán làm mượt (Smoothing) ---
		static const int s_HistorySize = 4; // Số lượng frame dùng để tính trung bình (4 frame ~= 66ms độ trễ).
		static int s_CurrentDeltaTimeIndex; // Chỉ số hiện tại trong bộ đệm vòng (Ring Buffer).
		static std::array<float, s_HistorySize> s_DeltaTimeHistory; // Bộ đệm lưu lịch sử DeltaTime.

	};
}
