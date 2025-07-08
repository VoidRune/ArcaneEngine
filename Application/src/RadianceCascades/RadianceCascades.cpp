#include "RadianceCascades.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"
#include "ArcaneEngine/Core/Timer.h"
#include "ArcaneEngine/Core/Log.h"
#include "stb/stb_image_write.h"
#include <regex>

RadianceCascades::RadianceCascades(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();

	m_ComputeShader = std::make_unique<Arc::Shader>();
	m_ComputePipeline = std::make_unique<Arc::ComputePipeline>();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/radiance.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_ComputeShader.get(), shaderDesc);

	m_ResourceCache->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_ComputeShader.get()
	});
}

RadianceCascades::~RadianceCascades()
{

}

void RadianceCascades::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	lastTime = elapsedTime;

	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindComputePipeline(m_ComputePipeline->GetHandle());
			cmd->Dispatch(32, 32, 1);
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
	//m_Device->GetTimestampQuery()->QueryResults();
}

void RadianceCascades::SwapchainResized(void* presentQueue)
{

}

void RadianceCascades::RecompileShaders()
{

}

void RadianceCascades::WaitForFrameEnd()
{

}