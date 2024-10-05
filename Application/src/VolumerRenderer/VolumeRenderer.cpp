#include "VolumeRenderer.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"

VolumeRenderer::VolumeRenderer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();

	//std::unique_ptr<Arc::GpuImage> image = std::make_unique<Arc::GpuImage>();
	//p_ResourceCache->CreateGpuImage(image.get(), Arc::GpuImageDesc{
	//	.Extent = {windowDesc.Width, windowDesc.Height, 1},
	//	.Format = Arc::Format::R8G8B8A8_Unorm,
	//	.UsageFlags = Arc::ImageUsage::ColorAttachment | Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst,
	//	.AspectFlags = Arc::ImageAspect::Color,
	//	.MipLevels = 1,
	//	});

	m_BufferArray = std::make_unique<Arc::GpuBufferArray>();
	m_ResourceCache->CreateGpuBufferArray(m_BufferArray.get(), Arc::GpuBufferDesc{
		.Size = sizeof(GlobalFrameData),
		.UsageFlags = Arc::BufferUsage::UniformBuffer,
		.MemoryProperty = Arc::MemoryProperty::HostVisible,
	});

	m_DescSetArray = std::make_unique<Arc::DescriptorSetArray>();
	m_ResourceCache->AllocateDescriptorSetArray(m_DescSetArray.get(), Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::Vertex }
		}
	});

	m_Device->UpdateDescriptorSet(m_DescSetArray.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::BufferArrayWrite(0, m_BufferArray.get()))
	);

	m_VertShader = std::make_unique<Arc::Shader>();
	m_FragShader = std::make_unique<Arc::Shader>();
	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/shader.vert", shaderDesc))
		m_ResourceCache->CreateShader(m_VertShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/shader.frag", shaderDesc))
		m_ResourceCache->CreateShader(m_FragShader.get(), shaderDesc);

	m_Pipeline = std::make_unique<Arc::Pipeline>();
	m_ResourceCache->CreatePipeline(m_Pipeline.get(), Arc::PipelineDesc{
		.ShaderStages = { m_VertShader.get(), m_FragShader.get() }
	});
}

VolumeRenderer::~VolumeRenderer()
{

}

void VolumeRenderer::RenderFrame(float elapsedTime)
{
	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	globalFrameData = { 0.2f, elapsedTime - (int)elapsedTime, 1.0f };
	{
		void* data = m_ResourceCache->MapMemory(m_BufferArray.get(), frameData.FrameIndex);
		memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
		m_ResourceCache->UnmapMemory(m_BufferArray.get(), frameData.FrameIndex);
	}

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {1, 0.5, 0, 1},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_Pipeline->GetLayout(), 0, { m_DescSetArray->GetHandle(frameData.FrameIndex)});
			cmd->BindPipeline(m_Pipeline->GetHandle());
			cmd->Draw(3, 1, 0, 0);
		}
	});
	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
}

void VolumeRenderer::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
}

void VolumeRenderer::RecompileShaders()
{

}

void VolumeRenderer::WaitForFrameEnd()
{

}