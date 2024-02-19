#include "MultiplayerGame.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include <fstream>
#include "stb/stb_image.h"
#include "Graphics/Vulkan/SpirvCompiler.h"
#include "Core/Log.h"
#include "Core/Timer.h"

MultiplayerGame::MultiplayerGame(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = core;
	m_PresentQueue = presentQueue;
	Arc::ImGuiInit(window->GetHandle(), core, presentQueue);

	m_WindowSize = presentQueue->GetSwapchain()->GetExtent();


	m_Server.Start(60000);

	CompileShaders();

	/* Prepare global data bound at the start of a frame */
	/* Camera */
	m_CameraFrameDataBuffer = std::make_unique<Arc::InFlightGpuBuffer>();
	m_Device->GetResourceCache()->CreateInFlightBuffer(m_CameraFrameDataBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(CameraFrameData))
		.AddBufferUsage(Arc::BufferUsage::UniformBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));
	/* Corresponding descriptor */
	m_GlobalDescriptor = std::make_unique<Arc::InFlightDescriptorSet>();
	m_Device->GetResourceCache()->AllocateInFlightDescriptorSet(m_GlobalDescriptor.get(), Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::VertexCompute));
	m_Device->UpdateInFlightDescriptorSet(m_GlobalDescriptor.get(), Arc::InFlightDescriptorWriteDesc()
		.AddBufferWrite(0, Arc::DescriptorType::UniformBuffer, m_CameraFrameDataBuffer->GetHandle(), 0, sizeof(CameraFrameData)));

	m_PointSampler = std::make_unique<Arc::Sampler>();
	core->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest));
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	core->GetResourceCache()->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Linear)
		.SetMagFilter(Arc::Filter::Linear)
		.SetAddressMode(Arc::SamplerAddressMode::MirroredRepeat));

	m_BindlessTexturesDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_BindlessTexturesDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1024, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment)
		.AddFlag(Arc::DescriptorFlags::Bindless));


	m_ComputeAttachment = std::make_unique<Arc::Image>();
	m_AccumulationImage = std::make_unique<Arc::Image>();
	core->GetResourceCache()->CreateImage(m_AccumulationImage.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R32G32B32A32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::Storage));
	core->GetResourceCache()->CreateImage(m_ComputeAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::Storage)
		.AddUsageFlag(Arc::ImageUsage::Sampled));

	m_PointSampler = std::make_unique<Arc::Sampler>();
	core->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest));

	m_PostprocessDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_PostprocessDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment));
	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_ComputeDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_ComputeDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute));
	m_Device->UpdateDescriptorSet(m_ComputeDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(4, m_PointSampler->GetHandle(), m_AccumulationImage->GetImageView(), Arc::ImageLayout::General)
		.AddImageWrite(5, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_ComputeAttachment.get(), Arc::ImageLayout::General);

	BuildRenderGraph();

	PreparePipelines();

	m_Camera = std::make_unique<CameraFPS>(window);
	m_Camera->Position = glm::vec3(0.5, 0.5, -0.5f);
	m_Camera->MovementSpeed = 0.5f;

	m_RenderThreadFuture = std::promise<void>().get_future();
}

MultiplayerGame::~MultiplayerGame()
{
	m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
	Arc::ImGuiShutdown();
	m_Device->GetResourceCache()->PrintHeapBudgets();
	m_Device->GetResourceCache()->FreeResources();
}

void MultiplayerGame::RenderFrame(float elapsedTime)
{
	float dt = elapsedTime - m_LastTime;
	m_LastTime = elapsedTime;
	m_RenderThreadFuture.wait();

	Arc::ImGuiBeginFrame();
	m_Camera->Update(dt);

	m_CameraFrameData.projection = m_Camera->Projection;
	m_CameraFrameData.view = m_Camera->View;
	m_CameraFrameData.inverseProjection = m_Camera->InverseProjection;
	m_CameraFrameData.inverseView = m_Camera->InverseView;
	m_CameraFrameData.cameraPos = glm::vec4(m_Camera->Position, 0);
	m_CameraFrameData.cameraDir = glm::vec4(m_Camera->Forward, 0);
	m_CameraFrameData.frameIndex++;

	ImGui::Begin("Profiling", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	auto res = m_Device->GetGpuProfiler()->GetResults();
	for (auto& el : res)
	{
		std::string s = el.taskName + ": " + std::to_string(el.timeInMs) + "ms";
		ImGui::Text(s.c_str());
	}
	ImGui::End();


	ImGui::Begin("Volume settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

	bool val1 = ImGui::SliderInt("Sample count", &m_CameraFrameData.sampleCount, 1, 256);
	bool val2 = ImGui::SliderInt("Light sample count", &m_CameraFrameData.lightSampleCount, 1, 64);
	bool val3 = ImGui::ColorEdit3("Background", &m_CameraFrameData.backgroundColor.r);
	bool val4 = ImGui::SliderFloat("Density limit minimum", &m_CameraFrameData.densityLimitMin, 0.0f, 1.0f);
	bool val5 = ImGui::SliderFloat("Density limit maximum", &m_CameraFrameData.densityLimitMax, 0.0f, 1.0f);
	bool val6 = ImGui::SliderFloat("Absorption", &m_CameraFrameData.absorptionCoefficient, 0.0f, 12.0f);
	bool val7 = ImGui::SliderFloat("Light multiplier", &m_CameraFrameData.lightMultiplier, 0.0f, 8.0f);
	ImGui::End();
	
	if (m_Camera->HasMoved ||
		val1 ||
		val2 ||
		val3 ||
		val4 ||
		val5 ||
		val6 ||
		val7)
	{
		m_CameraFrameData.frameIndex = 1;
	}

	m_RenderThreadFuture = std::async([&]() {

		Arc::FrameData fd = m_PresentQueue->BeginFrame();

		void* copyData = m_Device->GetResourceCache()->MapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);
		memcpy(copyData, &m_CameraFrameData, sizeof(CameraFrameData));
		m_Device->GetResourceCache()->UnmapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);

		m_Device->GetRenderGraph()->SetRecordFunc(m_PresentPassProxy,
			[&](Arc::CommandBuffer* cb, uint32_t frameIndex) {
				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetHandle());
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, { m_PostprocessDescriptor->GetHandle() });
				cb->Draw(6, 1, 0, 0);

				Arc::ImGuiEndFrame(cb->GetHandle());
			});

		m_Device->GetRenderGraph()->Execute(fd, m_PresentQueue->GetSwapchain()->GetExtent());
		m_PresentQueue->EndFrame();

	});
}

void MultiplayerGame::CompileShaders()
{
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();
	SpirvHelper::Init();
	m_Device->GetResourceCache()->CreateShader(m_PresentVertexShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/VolumetricRendering/Present.vert"));
	m_Device->GetResourceCache()->CreateShader(m_PresentFragmentShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/VolumetricRendering/Present.frag"));
	SpirvHelper::Finalize();
}

void MultiplayerGame::PreparePipelines()
{
	Arc::VertexAttributes emptyAttributes({ }, 0);

	m_PresentPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_PresentPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_PresentVertexShader.get())
		.AddShaderStage(m_PresentFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleFan)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_PresentPassProxy))
		.SetVertexAttributes(emptyAttributes));

}

void MultiplayerGame::BuildRenderGraph()
{
	m_PresentPassProxy = m_Device->GetRenderGraph()->AddPass("present", Arc::PresentPassDesc()
		.SetFormat(m_PresentQueue->GetSwapchain()->GetSurfaceFormat().format)
		.SetImageViews(m_PresentQueue->GetSwapchain()->GetImageViews())
		.SetExtent(m_PresentQueue->GetSwapchain()->GetExtent()));

	m_Device->GetRenderGraph()->BuildGraph();

}

void MultiplayerGame::WaitForFrameEnd()
{
	m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
}

void MultiplayerGame::RecompileShaders()
{
	WaitForFrameEnd();

	Arc::Timer timer;

	ARC_LOG("Recompiled shaders! " + std::to_string(timer.elapsed_mili()) + "ms");

	m_CameraFrameData.frameIndex = 0;
}

void MultiplayerGame::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
	m_WindowSize = m_PresentQueue->GetSwapchain()->GetExtent();

	m_Device->GetResourceCache()->ReleaseResource(m_AccumulationImage.get());
	m_Device->GetResourceCache()->ReleaseResource(m_ComputeAttachment.get());

	m_Device->GetResourceCache()->CreateImage(m_AccumulationImage.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R32G32B32A32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::Storage));
	m_Device->GetResourceCache()->CreateImage(m_ComputeAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::Storage)
		.AddUsageFlag(Arc::ImageUsage::Sampled));


	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->UpdateDescriptorSet(m_ComputeDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(4, m_PointSampler->GetHandle(), m_AccumulationImage->GetImageView(), Arc::ImageLayout::General)
		.AddImageWrite(5, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_ComputeAttachment.get(), Arc::ImageLayout::General);

	BuildRenderGraph();

	m_CameraFrameData.frameIndex = 0;
}