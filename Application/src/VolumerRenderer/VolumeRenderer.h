#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"

class VolumeRenderer : RendererBase
{
public:
	VolumeRenderer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~VolumeRenderer();

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
		float r, g, b;
	} globalFrameData;

	std::unique_ptr<Arc::GpuBufferArray> m_BufferArray;
	std::unique_ptr<Arc::DescriptorSetArray> m_DescSetArray;
	std::unique_ptr<Arc::Shader> m_VertShader;
	std::unique_ptr<Arc::Shader> m_FragShader;
	std::unique_ptr<Arc::Pipeline> m_Pipeline;

	std::unique_ptr<Arc::Shader> m_CompShader;
	std::unique_ptr<Arc::ComputePipeline> m_ComputePipeline;
	std::unique_ptr<Arc::GpuImage> m_ComputeImage;
	std::unique_ptr<Arc::DescriptorSet> m_ComputeDescriptor;

	std::unique_ptr<Arc::Sampler> m_LinearSampler;
};