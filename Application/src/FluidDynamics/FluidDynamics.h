#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include <glm/glm.hpp>

class FluidDynamics : public RendererBase
{
public:
	FluidDynamics(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~FluidDynamics();

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

	struct FluidData
	{
		glm::vec4 SourceColor;
		glm::vec2 SourcePos;
		glm::vec2 SourceVelocity;
		float SourceRadius;
		float DeltaTime;
	} fluidData;
	glm::vec2 m_LastMousePos;

	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;

	float m_Resolution = 0.5f;
	glm::ivec2 m_CanvasSize;
	glm::ivec2 m_ThreadDispatchSize;
	std::unique_ptr<Arc::GpuImage> m_Velocity1;
	std::unique_ptr<Arc::GpuImage> m_DivergencePressure1;
	std::unique_ptr<Arc::GpuImage> m_Dye1;
	std::unique_ptr<Arc::GpuImage> m_Velocity2;
	std::unique_ptr<Arc::GpuImage> m_DivergencePressure2;
	std::unique_ptr<Arc::GpuImage> m_Dye2;

	std::unique_ptr<Arc::Shader> m_AddForcesShader;
	std::unique_ptr<Arc::ComputePipeline> m_AddForcesPipeline;
	std::unique_ptr<Arc::Shader> m_DiffusionShader;
	std::unique_ptr<Arc::ComputePipeline> m_DiffusionPipeline;
	std::unique_ptr<Arc::Shader> m_AdvectionShader;
	std::unique_ptr<Arc::ComputePipeline> m_AdvectionPipeline;
	std::unique_ptr<Arc::Shader> m_DivergenceShader;
	std::unique_ptr<Arc::ComputePipeline> m_DivergencePipeline;
	std::unique_ptr<Arc::Shader> m_JacobiIterationShader;
	std::unique_ptr<Arc::ComputePipeline> m_JacobiIterationPipeline;
	std::unique_ptr<Arc::Shader> m_ProjectionShader;
	std::unique_ptr<Arc::ComputePipeline> m_ProjectionPipeline;

	std::unique_ptr<Arc::Shader> m_PresentVertShader;
	std::unique_ptr<Arc::Shader> m_PresentFragShader;
	std::unique_ptr<Arc::Pipeline> m_PresentPipeline;
};