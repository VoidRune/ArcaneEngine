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
	m_CompositeVertShader = std::make_unique<Arc::Shader>();
	m_CompositeFragShader = std::make_unique<Arc::Shader>();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/radiance.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_ComputeShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/shader.vert", shaderDesc))
		m_ResourceCache->CreateShader(m_CompositeVertShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/shader.frag", shaderDesc))
		m_ResourceCache->CreateShader(m_CompositeFragShader.get(), shaderDesc);

	m_NearestSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_NearestSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Nearest,
		.MagFilter = Arc::Filter::Nearest,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});

	m_OutputImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_OutputImage.get(), Arc::GpuImageDesc{
		.Extent = { m_PresentQueue->GetExtent()[0], m_PresentQueue->GetExtent()[1], 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});
	m_Device->TransitionImageLayout(m_OutputImage.get(), Arc::ImageLayout::General);

	m_ComputePipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_ComputeShader.get(),
		.UsePushDescriptors = true
	});

	m_GlobalDataBuffer = std::make_unique<Arc::GpuBufferArray>();
	m_ResourceCache->CreateGpuBufferArray(m_GlobalDataBuffer.get(), Arc::GpuBufferDesc{
		.Size = sizeof(GlobalFrameData),
		.UsageFlags = Arc::BufferUsage::UniformBuffer,
		.MemoryProperty = Arc::MemoryProperty::HostVisible,
	});

	m_CompositePipeline = std::make_unique<Arc::Pipeline>();;
	m_ResourceCache->CreatePipeline(m_CompositePipeline.get(), Arc::PipelineDesc{
		.ShaderStages = { m_CompositeVertShader.get(), m_CompositeFragShader.get() },
		.Topology = Arc::PrimitiveTopology::TriangleFan,
		.UsePushDescriptors = true
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

	globalFrameData.position = { Arc::Input::GetMouseX(), Arc::Input::GetMouseY() };
	globalFrameData.radius = Arc::Input::IsKeyDown(Arc::KeyCode::MouseLeft) ? 5.0f : 0.0f;
	globalFrameData.clear = Arc::Input::IsKeyPressed(Arc::KeyCode::C) ? 1.0f : 0.0f;
	globalFrameData.color = { 0, 1, 0, 1 };
	{
		void* data = m_ResourceCache->MapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
		memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
		m_ResourceCache->UnmapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
	}

	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindComputePipeline(m_ComputePipeline->GetHandle());
			cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_ComputePipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
				.AddWrite(Arc::PushBufferWrite(0, Arc::DescriptorType::UniformBuffer, m_GlobalDataBuffer->GetHandle(frameIndex), m_GlobalDataBuffer->GetSize()))
				.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_OutputImage->GetImageView(), Arc::ImageLayout::General, nullptr)));
			cmd->Dispatch(std::ceil(m_OutputImage->GetExtent()[0] / 32.0f), std::ceil(m_OutputImage->GetExtent()[1] / 32.0f), 1);
		}
	});

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {0.1, 0.6, 0.6, 1.0},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindPipeline(m_CompositePipeline->GetHandle());
			cmd->PushDescriptorSets(Arc::PipelineBindPoint::Graphics, m_CompositePipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
				.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::CombinedImageSampler, m_OutputImage->GetImageView(), Arc::ImageLayout::General, m_NearestSampler->GetHandle())));
			cmd->Draw(6, 1, 0, 0);
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