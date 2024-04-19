#pragma once
#include "Window/Window.h"
#include "Window/Input.h"
#include "Graphics/Device.h"
#include "Graphics/PresentQueue.h"

#include "Asset/AssetCache.h"
#include "RenderFrameData.h"
#include <future>

class Renderer
{
public:
	static void Initialize(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	static void Shutdown();

	static void RenderFrame();
	static void SwapchainResized(void* presentQueue);
	static void RecompileShaders();
	static void WaitForFrameEnd();

	static uint32_t BindTexture(Arc::GpuImage image);
	static void ReleaseTextureBinding(uint32_t binding);

	static RenderFrameData& GetFrameRenderData();
	static Arc::Device* GetDevice();
	static uint32_t GetWhiteTexture();
	static uint32_t GetBlackTexture();
private:

	static void CompileShaders();
	static void PreparePipelines();
	static void BuildRenderGraph();
};