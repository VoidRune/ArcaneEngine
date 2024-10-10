#include "VolumeRenderer.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"

VolumeRenderer::VolumeRenderer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();


	m_VertShader = std::make_unique<Arc::Shader>();
	m_FragShader = std::make_unique<Arc::Shader>();
	m_VolumeShader = std::make_unique<Arc::Shader>();
	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/shader.vert", shaderDesc))
		m_ResourceCache->CreateShader(m_VertShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/shader.frag", shaderDesc))
		m_ResourceCache->CreateShader(m_FragShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/compute.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_VolumeShader.get(), shaderDesc);

	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Linear,
		.MagFilter = Arc::Filter::Linear,
		.AddressMode = Arc::SamplerAddressMode::Repeat
	});

	m_OutputImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_OutputImage.get(), Arc::GpuImageDesc{
		.Extent = { m_PresentQueue->GetExtent()[0], m_PresentQueue->GetExtent()[1], 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});
	m_Device->TransitionImageLayout(m_OutputImage.get(), Arc::ImageLayout::General);

	m_AccumulationImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_AccumulationImage.get(), Arc::GpuImageDesc{
		.Extent = { m_PresentQueue->GetExtent()[0], m_PresentQueue->GetExtent()[1], 1},
		.Format = Arc::Format::R16G16B16A16_Sfloat,
		.UsageFlags = Arc::ImageUsage::Storage,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
		});
	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);


	m_GlobalDataBuffer = std::make_unique<Arc::GpuBufferArray>();
	m_ResourceCache->CreateGpuBufferArray(m_GlobalDataBuffer.get(), Arc::GpuBufferDesc{
		.Size = sizeof(GlobalFrameData),
		.UsageFlags = Arc::BufferUsage::UniformBuffer,
		.MemoryProperty = Arc::MemoryProperty::HostVisible,
	});

	m_GlobalDataDescSet = std::make_unique<Arc::DescriptorSetArray>();
	m_ResourceCache->AllocateDescriptorSetArray(m_GlobalDataDescSet.get(), Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::Compute }
		}
	});
	m_Device->UpdateDescriptorSet(m_GlobalDataDescSet.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::BufferArrayWrite(0, m_GlobalDataBuffer.get()))
	);

	m_VolumePipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_VolumePipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_VolumeShader.get()
	});

	m_VolumeImageDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_ResourceCache->AllocateDescriptorSet(m_VolumeImageDescriptor.get(), Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute }
		}
	});

	m_Device->UpdateDescriptorSet(m_VolumeImageDescriptor.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_AccumulationImage.get(), Arc::ImageLayout::General, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(1, m_OutputImage.get(), Arc::ImageLayout::General, m_LinearSampler.get()))
	);


	m_PresentPipeline = std::make_unique<Arc::Pipeline>();
	m_ResourceCache->CreatePipeline(m_PresentPipeline.get(), Arc::PipelineDesc{
		.ShaderStages = { m_VertShader.get(), m_FragShader.get() },
		.Topology = Arc::PrimitiveTopology::TriangleFan
	});

	m_PresentDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_ResourceCache->AllocateDescriptorSet(m_PresentDescriptor.get(), Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment }
		}
	});
	m_Device->UpdateDescriptorSet(m_PresentDescriptor.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_OutputImage.get(), Arc::ImageLayout::General, m_LinearSampler.get()))
	);
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
		void* data = m_ResourceCache->MapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
		memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
		m_ResourceCache->UnmapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
	}

	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindDescriptorSets(Arc::PipelineBindPoint::Compute, m_VolumePipeline->GetLayout(), 0, { m_GlobalDataDescSet->GetHandle(frameIndex), m_VolumeImageDescriptor->GetHandle()});
			cmd->BindComputePipeline(m_VolumePipeline->GetHandle());
			cmd->Dispatch(std::ceil(m_PresentQueue->GetExtent()[0] / 32.0f), std::ceil(m_PresentQueue->GetExtent()[1] / 32.0f), 1);
		}
	});
	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {1, 0.5, 0, 1},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, { m_PresentDescriptor->GetHandle() });
			cmd->BindPipeline(m_PresentPipeline->GetHandle());
			cmd->Draw(6, 1, 0, 0);
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