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

	struct GlobalFrameData
	{
		glm::vec2 position = {0, 0};
		float radius = 10.0f;
		float clear = 0.0f;
		glm::vec4 color = {0, 1, 0, 1};
	} globalFrameData;
	std::unique_ptr<Arc::Sampler> m_NearestSampler;

	std::unique_ptr<Arc::GpuBufferArray> m_GlobalDataBuffer;
	std::unique_ptr<Arc::GpuImage> m_OutputImage;

	std::unique_ptr<Arc::Shader> m_ComputeShader;
	std::unique_ptr<Arc::ComputePipeline> m_ComputePipeline;

	std::unique_ptr<Arc::Shader> m_CompositeVertShader;
	std::unique_ptr<Arc::Shader> m_CompositeFragShader;
	std::unique_ptr<Arc::Pipeline> m_CompositePipeline;
};