#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include <glm/glm.hpp>

class RadianceCascades : RendererBase
{
public:
	RadianceCascades(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~RadianceCascades();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;

	std::unique_ptr<Arc::Shader> m_ComputeShader;
	std::unique_ptr<Arc::ComputePipeline> m_ComputePipeline;
};