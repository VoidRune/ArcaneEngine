#pragma once

class BaseRenderer
{
public:
	BaseRenderer() {}
	virtual ~BaseRenderer() {}
	virtual void RenderFrame(float elapsedTime) {}
	virtual void SwapchainResized(void* presentQueue) {}
	virtual void WaitForFrameEnd() {}
};