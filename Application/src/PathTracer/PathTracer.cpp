#include "PathTracer.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"
#include "ArcaneEngine/Core/Log.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"

PathTracer::PathTracer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();

	CreatePipelines();
	CreateSamplers();
	CreateImages();
}


void PathTracer::CreatePipelines()
{

}

void PathTracer::CreateSamplers()
{
	m_NearestSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_NearestSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Nearest,
		.MagFilter = Arc::Filter::Nearest,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Linear,
		.MagFilter = Arc::Filter::Linear,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});
}

void PathTracer::CreateImages()
{
	uint32_t w = m_PresentQueue->GetExtent()[0];
	uint32_t h = m_PresentQueue->GetExtent()[1];

	m_Device->WaitIdle();
}

PathTracer::~PathTracer()
{

}

void PathTracer::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	lastTime = elapsedTime;

	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;


	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			
		}
	});

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {0.1, 0.6, 0.6, 1.0},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {

		}
	});

	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
	m_Device->WaitIdle();
}

void PathTracer::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
	CreateImages();
}

void PathTracer::RecompileShaders()
{

}

void PathTracer::WaitForFrameEnd()
{

}