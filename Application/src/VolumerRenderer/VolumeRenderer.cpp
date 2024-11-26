#include "VolumeRenderer.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"
#include "DataSetLoader.h"

VolumeRenderer::VolumeRenderer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();
	m_ImGuiRenderer = std::make_unique<Arc::ImGuiRenderer>(window, device, presentQueue);
	m_TransferFunctionEditor = std::make_unique<TransferFunctionEditor>();

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
		.AddressMode = Arc::SamplerAddressMode::ClampToEdge
	});

	m_NearestSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_NearestSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Nearest,
		.MagFilter = Arc::Filter::Nearest,
		.AddressMode = Arc::SamplerAddressMode::Repeat
	});

	m_DatasetImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_DatasetImage.get(), Arc::GpuImageDesc{
		.Extent = { 512, 512, 182 },
		.Format = Arc::Format::R8_Unorm,
		.UsageFlags = Arc::ImageUsage::Sampled | Arc::ImageUsage::TransferDst,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});

	m_MaxExtinctionImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_MaxExtinctionImage.get(), Arc::GpuImageDesc{
		.Extent = { m_ExtinctionGridSize, m_ExtinctionGridSize, m_ExtinctionGridSize },
		.Format = Arc::Format::R8_Unorm,
		.UsageFlags = Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled | Arc::ImageUsage::TransferDst,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});

	m_TransferFunctionImage = std::make_unique<Arc::GpuImage>();
	uint32_t transferFunctionSize = 256;
	m_ResourceCache->CreateGpuImage(m_TransferFunctionImage.get(), Arc::GpuImageDesc{
		.Extent = { transferFunctionSize, 1, 1 },
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::Sampled | Arc::ImageUsage::TransferDst,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});
	{
		m_DataSet = DatasetLoader::LoadFromFile("res/Datasets/bonsai.raw");
		m_Device->SetImageData(m_DatasetImage.get(), m_DataSet.data(), m_DataSet.size(), Arc::ImageLayout::ShaderReadOnlyOptimal);

		auto transferData = m_TransferFunctionEditor->GenerateTransferFunctionImage(transferFunctionSize);
		m_Device->SetImageData(m_TransferFunctionImage.get(), transferData.data(), transferData.size() * sizeof(uint32_t), Arc::ImageLayout::ShaderReadOnlyOptimal);
		
		auto extinctionData = m_TransferFunctionEditor->GetMaxExtinctionGrid(transferData, m_ExtinctionGridSize, m_DataSet, 512, 512, 182);
		std::vector<uint8_t> startingData(m_ExtinctionGridSize * m_ExtinctionGridSize * m_ExtinctionGridSize);
		for (auto& val : startingData) val = 0.001f;
		m_Device->SetImageData(m_MaxExtinctionImage.get(), extinctionData.data(), extinctionData.size() * sizeof(uint8_t), Arc::ImageLayout::General);
	}

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

	m_VolumeImageDescriptor1 = std::make_unique<Arc::DescriptorSet>();
	m_VolumeImageDescriptor2 = std::make_unique<Arc::DescriptorSet>();
	{
		Arc::DescriptorSetDesc desc = Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute },
			{ Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute }
		}};
		m_ResourceCache->AllocateDescriptorSet(m_VolumeImageDescriptor1.get(), desc);
		m_ResourceCache->AllocateDescriptorSet(m_VolumeImageDescriptor2.get(), desc);
	}


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

	m_Camera = std::make_unique<CameraFP>(m_Window);
	m_Camera->Position = glm::vec3(0.5, 0.5, -0.5f);
	m_Camera->MovementSpeed = 0.5f;
	globalFrameData.frameIndex = 1;

	m_ImGuiTransferImage = m_ImGuiRenderer->CreateImageId(m_TransferFunctionImage->GetImageView(), m_LinearSampler->GetHandle());
}

VolumeRenderer::~VolumeRenderer()
{

}

void VolumeRenderer::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	lastTime = elapsedTime;
	m_IsEvenFrame = !m_IsEvenFrame;

	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	globalFrameData.projection = m_Camera->Projection;
	globalFrameData.view = m_Camera->View;
	globalFrameData.inverseProjection = m_Camera->InverseProjection;
	globalFrameData.inverseView = m_Camera->InverseView;
	globalFrameData.cameraPosition = glm::vec4(m_Camera->Position, 0.0f);
	globalFrameData.cameraDirection = glm::vec4(m_Camera->Forward, 0.0f);
	globalFrameData.frameIndex++;

	m_ImGuiRenderer->BeginFrame();
	ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_AutoHideTabBar);
	ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoTitleBar);
	ImVec2 canvasRegion = ImGui::GetContentRegionAvail();

	m_Camera->Update(dt, canvasRegion.x / canvasRegion.y);
	if (m_ImGuiCanvasSize.x != canvasRegion.x || m_ImGuiCanvasSize.y != canvasRegion.y)
	{
		m_ImGuiCanvasSize.x = canvasRegion.x;
		m_ImGuiCanvasSize.y = canvasRegion.y;
		globalFrameData.frameIndex = 1;
		ResizeCanvas(m_ImGuiCanvasSize.x, m_ImGuiCanvasSize.y);
	}

	ImGui::Image(m_ImGuiDisplayImage, canvasRegion);
	ImGui::End();
	m_TransferFunctionEditor->Render(m_ImGuiTransferImage);
	ImGui::Begin("Volume Settings", nullptr, ImGuiWindowFlags_NoCollapse);
	bool guiChange = ImGui::SliderInt("Bounce limit", &globalFrameData.bounceLimit, 0, 128);
	guiChange |= ImGui::SliderFloat("Extinction", &globalFrameData.extinction, 0.1f, 300.0f);
	guiChange |= ImGui::SliderFloat("Anisotropy", &globalFrameData.anisotropy, -1.0f, 1.0f);
	guiChange |= ImGui::ColorEdit3("Background", &globalFrameData.backgroundColor.r);
	ImGui::End();

	if (guiChange || m_Camera->HasMoved || m_TransferFunctionEditor->HasDataChanged())
	{
		globalFrameData.frameIndex = 1;
	}
	{
		void* data = m_ResourceCache->MapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
		memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
		m_ResourceCache->UnmapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
	
		if (m_TransferFunctionEditor->HasDataChanged())
		{
			auto transferData = m_TransferFunctionEditor->GenerateTransferFunctionImage(m_TransferFunctionImage->GetExtent()[0]);
			m_Device->SetImageData(m_TransferFunctionImage.get(), transferData.data(), transferData.size() * sizeof(uint32_t), Arc::ImageLayout::ShaderReadOnlyOptimal);
			
			//std::vector<uint8_t> startingData(m_ExtinctionGridSize * m_ExtinctionGridSize * m_ExtinctionGridSize);
			//for (auto& val : startingData) val = 0.001f;
			//m_Device->SetImageData(m_MaxExtinctionImage1.get(), startingData.data(), startingData.size() * sizeof(uint8_t), Arc::ImageLayout::General);	
			//m_Device->SetImageData(m_MaxExtinctionImage2.get(), startingData.data(), startingData.size() * sizeof(uint8_t), Arc::ImageLayout::General);
		}
	}

	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->MemoryBarrier({
			Arc::CommandBuffer::ImageBarrier{
				.Handle = m_OutputImage->GetHandle(),
				.OldLayout = Arc::ImageLayout::Undefined,
				.NewLayout = Arc::ImageLayout::General,
			} });
			cmd->BindDescriptorSets(Arc::PipelineBindPoint::Compute, m_VolumePipeline->GetLayout(), 0, { m_GlobalDataDescSet->GetHandle(frameIndex), m_IsEvenFrame ? m_VolumeImageDescriptor1->GetHandle() : m_VolumeImageDescriptor2->GetHandle() });
			cmd->BindComputePipeline(m_VolumePipeline->GetHandle());
			cmd->Dispatch(std::ceil(m_ImGuiCanvasSize.x / 32.0f), std::ceil(m_ImGuiCanvasSize.y / 32.0f), 1);
		
			cmd->MemoryBarrier({
			Arc::CommandBuffer::ImageBarrier{
				.Handle = m_OutputImage->GetHandle(),
				.OldLayout = Arc::ImageLayout::General,
				.NewLayout = Arc::ImageLayout::ShaderReadOnlyOptimal
			} });
		}
	});
	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {1, 0.5, 1, 1},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			//cmd->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, { m_PresentDescriptor->GetHandle() });
			//cmd->BindPipeline(m_PresentPipeline->GetHandle());
			//cmd->Draw(6, 1, 0, 0);
			m_ImGuiRenderer->EndFrame(cmd->GetHandle());
		}
	});
	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
}

void VolumeRenderer::ResizeCanvas(uint32_t width, uint32_t height)
{
	m_Device->WaitIdle();
	if(m_OutputImage.get())
		m_ResourceCache->ReleaseResource(m_OutputImage.get());
	m_OutputImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_OutputImage.get(), Arc::GpuImageDesc{
		.Extent = { width, height, 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});
	if (m_AccumulationImage1.get())
		m_ResourceCache->ReleaseResource(m_AccumulationImage1.get());
	if (m_AccumulationImage2.get())
		m_ResourceCache->ReleaseResource(m_AccumulationImage2.get());
	m_AccumulationImage1 = std::make_unique<Arc::GpuImage>();
	m_AccumulationImage2 = std::make_unique<Arc::GpuImage>();
	{
		Arc::GpuImageDesc desc = Arc::GpuImageDesc{
		.Extent = { m_PresentQueue->GetExtent()[0], m_PresentQueue->GetExtent()[1], 1},
		.Format = Arc::Format::R32G32B32A32_Sfloat,
		.UsageFlags = Arc::ImageUsage::Storage,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
		};
		m_ResourceCache->CreateGpuImage(m_AccumulationImage1.get(), desc);
		m_ResourceCache->CreateGpuImage(m_AccumulationImage2.get(), desc);
		m_Device->TransitionImageLayout(m_AccumulationImage1.get(), Arc::ImageLayout::General);
		m_Device->TransitionImageLayout(m_AccumulationImage2.get(), Arc::ImageLayout::General);
	}

	m_Device->UpdateDescriptorSet(m_VolumeImageDescriptor1.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_AccumulationImage1.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(1, m_AccumulationImage2.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(2, m_OutputImage.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(3, m_DatasetImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(4, m_TransferFunctionImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(5, m_MaxExtinctionImage.get(), Arc::ImageLayout::General, m_NearestSampler.get()))
		.AddWrite(Arc::ImageWrite(6, m_MaxExtinctionImage.get(), Arc::ImageLayout::General, nullptr))
	);
	m_Device->UpdateDescriptorSet(m_VolumeImageDescriptor2.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_AccumulationImage2.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(1, m_AccumulationImage1.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(2, m_OutputImage.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(3, m_DatasetImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(4, m_TransferFunctionImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(5, m_MaxExtinctionImage.get(), Arc::ImageLayout::General, m_NearestSampler.get()))
		.AddWrite(Arc::ImageWrite(6, m_MaxExtinctionImage.get(), Arc::ImageLayout::General, nullptr))
	);

	m_Device->UpdateDescriptorSet(m_PresentDescriptor.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_OutputImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_NearestSampler.get()))
	);

	m_ImGuiDisplayImage = m_ImGuiRenderer->CreateImageId(m_OutputImage->GetImageView(), m_LinearSampler->GetHandle());
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