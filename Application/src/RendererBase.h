#pragma once

class RendererBase
{
public:
	virtual void RenderFrame(float elapsedTime) {}
	virtual void SwapchainResized(void* presentQueue) {}
	virtual void RecompileShaders() {}
	virtual void WaitForFrameEnd() {}
};