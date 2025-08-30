#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include <glm/glm.hpp>

class PathTracer : public RendererBase
{
public:
	PathTracer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~PathTracer();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:

	void CreatePipelines();
	void CreateSamplers();
	void CreateImages();

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;

	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
};