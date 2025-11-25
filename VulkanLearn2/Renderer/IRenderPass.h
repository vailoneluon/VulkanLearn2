#pragma once
#include "pch.h"

// =================================================================================================
// Class: IRenderPass
// Mô tả: Interface chung cho tất cả các render pass trong hệ thống.
//        Đảm bảo tính đa hình và khả năng mở rộng của renderer.
// =================================================================================================
class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	// Phương thức thuần ảo để thực thi pass render.
	// cmdBuffer: Command buffer để ghi lệnh vẽ.
	// imageIndex: Index của swapchain image hiện tại (nếu cần).
	// currentFrame: Index của frame hiện tại (cho synchronization).
	virtual void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) = 0;

private:

};
