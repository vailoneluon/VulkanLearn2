#pragma once
#include "pch.h"

class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	virtual void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) = 0;


private:

};
