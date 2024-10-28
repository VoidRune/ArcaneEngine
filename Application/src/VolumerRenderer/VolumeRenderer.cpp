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
		.AddressMode = Arc::SamplerAddressMode::Repeat
	});

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
		.UsageFlags = Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});
	//m_Device->TransitionImageLayout(m_OutputImage.get(), Arc::ImageLayout::General);

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

	m_DatasetImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_DatasetImage.get(), Arc::GpuImageDesc{
		.Extent = { 512, 512, 182 },
		.Format = Arc::Format::R8_Unorm,
		.UsageFlags = Arc::ImageUsage::Sampled | Arc::ImageUsage::TransferDst,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
	});
	{
		std::vector<uint8_t> dataSet = DatasetLoader::LoadFromFile("res/Datasets/bonsai.raw");
		m_Device->SetImageData(m_DatasetImage.get(), dataSet.data(), dataSet.size(), Arc::ImageLayout::ShaderReadOnlyOptimal);
	}

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
		std::vector<uint32_t> data(transferFunctionSize);
		for (size_t i = 0; i < data.size(); i++)
		{
			int r = 0;
			int g = 0;
			int b = 255;
			int a = i;
			data[i] = a << 24 | b << 16 | g << 8 | r;
		}
		m_Device->SetImageData(m_TransferFunctionImage.get(), data.data(), data.size() * sizeof(uint32_t), Arc::ImageLayout::ShaderReadOnlyOptimal);
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
			{ Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute }
		}};
		m_ResourceCache->AllocateDescriptorSet(m_VolumeImageDescriptor1.get(), desc);
		m_ResourceCache->AllocateDescriptorSet(m_VolumeImageDescriptor2.get(), desc);
	}

	m_Device->UpdateDescriptorSet(m_VolumeImageDescriptor1.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_AccumulationImage1.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(1, m_AccumulationImage2.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(2, m_OutputImage.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(3, m_DatasetImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(4, m_TransferFunctionImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
	);
	m_Device->UpdateDescriptorSet(m_VolumeImageDescriptor2.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::ImageWrite(0, m_AccumulationImage2.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(1, m_AccumulationImage1.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(2, m_OutputImage.get(), Arc::ImageLayout::General, nullptr))
		.AddWrite(Arc::ImageWrite(3, m_DatasetImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(4, m_TransferFunctionImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
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
		.AddWrite(Arc::ImageWrite(0, m_OutputImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_NearestSampler.get()))
	);

	m_Camera = std::make_unique<CameraFP>(m_Window);
	m_Camera->Position = glm::vec3(0.5, 0.5, -0.5f);
	m_Camera->MovementSpeed = 0.5f;
	globalFrameData.frameIndex = 1;

	m_ImGuiDisplayImage = m_ImGuiRenderer->CreateImageId(m_OutputImage->GetImageView(), m_LinearSampler->GetHandle());
}

VolumeRenderer::~VolumeRenderer()
{

}

void VolumeRenderer::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	m_Camera->Update(dt);
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
	globalFrameData.backgroundColor = glm::vec3(1, 1, 1);
	globalFrameData.frameIndex++;
	globalFrameData.bounceLimit = 8;
	globalFrameData.anisotropy = 0.2f;
	globalFrameData.extinction = 8.0f;

	if (m_Camera->HasMoved)
	{
		globalFrameData.frameIndex = 1;
	}

	{
		void* data = m_ResourceCache->MapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
		memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
		m_ResourceCache->UnmapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
	
		auto transferData = m_TransferFunctionEditor->GenerateTransferFunctionImage(256);
		m_Device->SetImageData(m_TransferFunctionImage.get(), transferData.data(), transferData.size() * sizeof(uint32_t), Arc::ImageLayout::ShaderReadOnlyOptimal);
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
			cmd->Dispatch(std::ceil(m_PresentQueue->GetExtent()[0] / 32.0f), std::ceil(m_PresentQueue->GetExtent()[1] / 32.0f), 1);
		
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
			cmd->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, { m_PresentDescriptor->GetHandle() });
			cmd->BindPipeline(m_PresentPipeline->GetHandle());
			cmd->Draw(6, 1, 0, 0);

			//ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

			m_ImGuiRenderer->BeginFrame();
			ImGui::DockSpaceOverViewport();
			//ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
			ImGui::ShowDemoWindow();			
			ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoTitleBar);
			ImGui::Image(m_ImGuiDisplayImage, ImGui::GetContentRegionAvail());
			ImGui::End();
			m_TransferFunctionEditor->Render();
			//ImDrawList* drawlist = ImGui::GetWindowDrawList();
			//drawlist->AddCircleFilled({ 30, 30 }, 5.0f, IM_COL32(100, 255, 0, 200));
			//drawlist->AddCircleFilled({ 60, 50 }, 5.0f, IM_COL32(100, 255, 0, 200));
			//ImGui::End();
			m_ImGuiRenderer->EndFrame(cmd->GetHandle());
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