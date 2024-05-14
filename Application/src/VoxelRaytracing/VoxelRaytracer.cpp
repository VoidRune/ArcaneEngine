#include "VoxelRaytracer.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include "stb/stb_image.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Graphics/Vulkan/ShaderCompiler.h"
#include <fstream>

VoxelRaytracer::VoxelRaytracer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	Arc::ImGuiInit(window->GetHandle(), device, presentQueue);

	m_WindowSize = presentQueue->GetSwapchain()->GetExtent();

	CompileShaders();

	/* Prepare global data bound at the start of a frame */
	/* Camera */
	m_CameraFrameDataBuffer = std::make_unique<Arc::GpuBufferSet>();
	m_Device->GetResourceCache()->CreateBufferSet(m_CameraFrameDataBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(CameraFrameData))
		.AddBufferUsage(Arc::BufferUsage::UniformBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));
	/* Corresponding descriptor */
	m_GlobalDescriptor = std::make_unique<Arc::DescriptorSetArray>();
	m_Device->GetResourceCache()->AllocateDescriptorSetArray(m_GlobalDescriptor.get(), Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::VertexCompute));
	m_Device->UpdateDescriptorSetArray(m_GlobalDescriptor.get(), Arc::DescriptorArrayWriteDesc()
		.AddBufferWrite(0, m_CameraFrameDataBuffer->GetHandle(), 0, sizeof(CameraFrameData)));

	m_PointSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest));
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Linear)
		.SetMagFilter(Arc::Filter::Linear)
		.SetAddressMode(Arc::SamplerAddressMode::MirroredRepeat));


	uint32_t worldSize = 16;
	m_VoxelDataImage = std::make_unique<Arc::GpuImage>();
	m_Device->GetResourceCache()->CreateImage(m_VoxelDataImage.get(), Arc::GpuImageDesc()
		.SetExtent({ worldSize, worldSize })
		.SetDepth(worldSize)
		.SetFormat(Arc::Format::R32_Sfloat)
		.SetEnableMipLevels(false)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));

	uint32_t gradSize = 256;
	m_GradientData.resize(gradSize * 4);

	m_ColorGradientImage = std::make_unique<Arc::GpuImage>();
	m_Device->GetResourceCache()->CreateImage(m_ColorGradientImage.get(), Arc::GpuImageDesc()
		.SetExtent({ (uint32_t)gradSize, (uint32_t)1 })
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	m_Device->SetImageData(m_ColorGradientImage.get(), m_GradientData.data(), gradSize * 1 * 4, Arc::ImageLayout::ShaderReadOnlyOptimal);

	m_ImGuiDset = ImGui_ImplVulkan_AddTexture(m_LinearSampler->GetHandle(), m_ColorGradientImage->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	std::vector<float> voxels;
	uint32_t maxVoxels = worldSize * worldSize * worldSize;

	for (size_t z = 0; z < worldSize; z++)
		for (size_t y = 0; y < worldSize; y++)
			for (size_t x = 0; x < worldSize; x++)
			{
				if (x == 0 || x == worldSize - 1 ||
					y == 0 || y == worldSize - 1 ||
					z == 0 || z == worldSize - 1 ||
					x == 5 && z == 5)
				{

					voxels.push_back(1.0f);
				}
				else
				{
					voxels.push_back(0.0f);
				}
			}

	m_Device->SetImageData(m_VoxelDataImage.get(), voxels.data(), voxels.size() * sizeof(float), Arc::ImageLayout::ShaderReadOnlyOptimal);


	m_ComputeAttachment = std::make_unique<Arc::GpuImage>();
	m_AccumulationImage = std::make_unique<Arc::GpuImage>();
	m_Device->GetResourceCache()->CreateImage(m_AccumulationImage.get(), Arc::GpuImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R32G32B32A32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::Storage));
	m_Device->GetResourceCache()->CreateImage(m_ComputeAttachment.get(), Arc::GpuImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::Storage)
		.AddUsageFlag(Arc::ImageUsage::Sampled));

	m_PointSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest)
		.SetAddressMode(Arc::SamplerAddressMode::MirroredRepeat));

	m_PostprocessDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_PostprocessDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment));
	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_ComputeDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_ComputeDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute));
	m_Device->UpdateDescriptorSet(m_ComputeDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_LinearSampler->GetHandle(), m_ColorGradientImage->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(1, m_PointSampler->GetHandle(), m_VoxelDataImage->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(2, m_PointSampler->GetHandle(), m_AccumulationImage->GetImageView(), Arc::ImageLayout::General)
		.AddImageWrite(3, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_ComputeAttachment.get(), Arc::ImageLayout::General);

	BuildRenderGraph();

	PreparePipelines();

	m_Camera = std::make_unique<CameraFPS>(window);
	m_Camera->Position = glm::vec3(0.5, 0.5, -0.5f);
	m_Camera->MovementSpeed = 0.5f;

	m_RenderThreadFuture = std::promise<void>().get_future();
}

VoxelRaytracer::~VoxelRaytracer()
{
	m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
	Arc::ImGuiShutdown();
	m_Device->GetResourceCache()->PrintHeapBudgets();
	m_Device->GetResourceCache()->FreeResources();
}

void VoxelRaytracer::RenderFrame(float elapsedTime)
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
	ImGui::Image(m_ImGuiDset, ImVec2(256, 20));


	ImGui::SameLine();
	ImGui::Text("Density point");

	bool val1 = ImGui::SliderInt("Bounce limit", &m_CameraFrameData.bounceLimit, 0, 128);
	bool val2 = ImGui::SliderFloat("Extinction", &m_CameraFrameData.extinction, 0.1f, 300.0f);
	bool val3 = ImGui::SliderFloat("Anisotropy", &m_CameraFrameData.anisotropy, -1.0f, 1.0f);
	bool val4 = ImGui::ColorEdit3("Background", &m_CameraFrameData.backgroundColor.r);
	ImGui::End();

	if (m_Camera->HasMoved ||
		val1 ||
		val2 ||
		val3 ||
		val4)
	{
		m_CameraFrameData.frameIndex = 1;
	}

	m_RenderThreadFuture = std::async([&]() {

		Arc::FrameData fd = m_PresentQueue->BeginFrame();

		void* copyData = m_Device->GetResourceCache()->MapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);
		memcpy(copyData, &m_CameraFrameData, sizeof(CameraFrameData));
		m_Device->GetResourceCache()->UnmapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);

		m_Device->GetRenderGraph()->SetRecordFunc(m_ComputePassProxy,
			[&](Arc::CommandBuffer* cb, uint32_t frameIndex) {
				cb->BindPipeline(Arc::PipelineBindPoint::Compute, m_ComputePipeline->GetHandle());
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Compute, m_ComputePipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(frameIndex), m_ComputeDescriptor->GetHandle() });
				cb->Dispatch(std::ceil(m_WindowSize.width / 32.0), std::ceil(m_WindowSize.height / 32.0), 1);
			});

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

void VoxelRaytracer::CompileShaders()
{
	m_RaytraceComputeShader = std::make_unique<Arc::Shader>();
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();

	Arc::ShaderCompiler::Initialize();
	Arc::ShaderDesc shaderDesc;
	Arc::ShaderCompiler::Compile("res/Shaders/VoxelRaytracing/Raytrace.comp", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_RaytraceComputeShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/VoxelRaytracing/Present.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_PresentVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/VoxelRaytracing/Present.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_PresentFragmentShader.get(), shaderDesc);
	Arc::ShaderCompiler::Finalize();

}

void VoxelRaytracer::PreparePipelines()
{
	Arc::VertexAttributes emptyAttributes({ }, 0);

	m_ComputePipeline = std::make_unique<Arc::ComputePipeline>();
	m_Device->GetResourceCache()->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc()
		.SetShaderStage(m_RaytraceComputeShader.get()));

	m_PresentPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_PresentPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_PresentVertexShader.get())
		.AddShaderStage(m_PresentFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleFan)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_PresentPassProxy))
		.SetVertexAttributes(emptyAttributes));

}

void VoxelRaytracer::BuildRenderGraph()
{
	m_ComputePassProxy = m_Device->GetRenderGraph()->AddPass("compute", Arc::ComputePassDesc()
		.SetOutputImages({ m_AccumulationImage->GetProxy() }));

	m_PresentPassProxy = m_Device->GetRenderGraph()->AddPass("present", Arc::PresentPassDesc()
		.SetFormat(m_PresentQueue->GetSwapchain()->GetSurfaceFormat().format)
		.SetImageViews(m_PresentQueue->GetSwapchain()->GetImageViews())
		.SetExtent(m_PresentQueue->GetSwapchain()->GetExtent()));

	m_Device->GetRenderGraph()->BuildGraph();

}

void VoxelRaytracer::WaitForFrameEnd()
{
	m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
}

void VoxelRaytracer::RecompileShaders()
{
	WaitForFrameEnd();

	Arc::Timer timer;

	Arc::ShaderCompiler::Initialize();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/VoxelRaytracing/Raytrace.comp", shaderDesc))
	{
		m_Device->GetResourceCache()->ReleaseResource(m_RaytraceComputeShader.get());
		m_Device->GetResourceCache()->CreateShader(m_RaytraceComputeShader.get(), shaderDesc);

		m_Device->GetResourceCache()->ReleaseResource(m_ComputePipeline.get());
		m_Device->GetResourceCache()->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc()
			.SetShaderStage(m_RaytraceComputeShader.get()));
	}

	Arc::ShaderCompiler::Finalize();

	ARC_LOG("Recompiled shaders! " + std::to_string(timer.elapsed_mili()) + "ms");

	m_CameraFrameData.frameIndex = 0;
}

void VoxelRaytracer::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
	m_WindowSize = m_PresentQueue->GetSwapchain()->GetExtent();

	m_Device->GetResourceCache()->ReleaseResource(m_AccumulationImage.get());
	m_Device->GetResourceCache()->ReleaseResource(m_ComputeAttachment.get());

	m_Device->GetResourceCache()->CreateImage(m_AccumulationImage.get(), Arc::GpuImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R32G32B32A32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::Storage));
	m_Device->GetResourceCache()->CreateImage(m_ComputeAttachment.get(), Arc::GpuImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::Storage)
		.AddUsageFlag(Arc::ImageUsage::Sampled));


	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->UpdateDescriptorSet(m_ComputeDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(3, m_PointSampler->GetHandle(), m_AccumulationImage->GetImageView(), Arc::ImageLayout::General)
		.AddImageWrite(4, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_ComputeAttachment.get(), Arc::ImageLayout::General);

	BuildRenderGraph();

	m_CameraFrameData.frameIndex = 0;
}