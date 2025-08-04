#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include <glm/glm.hpp>

class RadianceCascades : public RendererBase
{
public:
	RadianceCascades(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~RadianceCascades();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:

	void CreatePipelines();
	void CreateSamplers();
	void CreateImages();
	glm::vec3 HsvToRgb(float h, float s, float v);

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;

	struct CascadeData
	{
		float Index = 0.0f;
		float Interval = 1.0f;
		float Count = 0.0f;
	} cascadeData;
	struct JfaData
	{
		glm::vec2 position = { 0, 0 };
		float radius = 10.0f;
		float clear = 0.0f;
		glm::vec4 color = { 0, 1, 0, 1 };
		float iteration = 0.0f;
	} jfaData;

	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;

	std::unique_ptr<Arc::GpuImage> m_SeedImage;
	std::unique_ptr<Arc::GpuImage> m_JFAImage;
	std::unique_ptr<Arc::GpuImage> m_SDFImage;
	std::unique_ptr<Arc::GpuImage> m_NearestColorImage;
	std::vector<Arc::GpuImage> m_Cascades;

	std::unique_ptr<Arc::Shader> m_JFAShader;
	std::unique_ptr<Arc::ComputePipeline> m_JFAPipeline;
	std::unique_ptr<Arc::Shader> m_RadianceCascadesShader;
	std::unique_ptr<Arc::ComputePipeline> m_RadianceCascadesPipeline;

	std::unique_ptr<Arc::Shader> m_CompositeVertShader;
	std::unique_ptr<Arc::Shader> m_CompositeFragShader;
	std::unique_ptr<Arc::Pipeline> m_CompositePipeline;
};