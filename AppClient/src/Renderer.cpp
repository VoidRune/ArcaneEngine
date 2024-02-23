#include "Renderer.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include "stb/stb_image.h"
#include "Graphics/Vulkan/SpirvCompiler.h"
#include "Core/Log.h"
#include "Core/Timer.h"

Renderer::Renderer(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = core;
	m_PresentQueue = presentQueue;
	Arc::ImGuiInit(window->GetHandle(), core, presentQueue);

	m_WindowSize = presentQueue->GetSwapchain()->GetExtent();

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
		.AddBinding(1, Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::Vertex));
	m_Device->UpdateInFlightDescriptorSet(m_GlobalDescriptor.get(), Arc::InFlightDescriptorWriteDesc()
		.AddBufferWrite(0, Arc::DescriptorType::UniformBuffer, m_CameraFrameDataBuffer->GetHandle(), 0, sizeof(CameraFrameData)));

	m_ColorAttachment = std::make_unique<Arc::Image>();
	m_DepthAttachment = std::make_unique<Arc::Image>();

	m_Device->GetResourceCache()->CreateImage(m_ColorAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R16G16B16A16_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::ColorAttachment)
		.AddUsageFlag(Arc::ImageUsage::Sampled)
		.AddUsageFlag(Arc::ImageUsage::Storage));
	m_Device->GetResourceCache()->CreateImage(m_DepthAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::D32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::DepthStencilAttachment)
		.AddUsageFlag(Arc::ImageUsage::Sampled));

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


	//m_Device->UpdateDescriptorSet(m_BindlessTexturesDescriptor.get(), Arc::DescriptorWriteDesc()
	//	.AddImageWrite(0, m_LinearSampler->GetHandle(), texture.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal, 1));

	m_PointSampler = std::make_unique<Arc::Sampler>();
	core->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest));

	m_PostprocessDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_PostprocessDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment));
	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ColorAttachment->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal));

	BuildRenderGraph();

	PreparePipelines();

	//m_RenderThreadFuture = std::promise<void>().get_future();

	std::vector<StaticVertex> vertices;
	vertices.push_back(StaticVertex({-1,-1, 0 }, { 1, 0, 0 }, { 0, 0 }));
	vertices.push_back(StaticVertex({ 0, 1, 0 }, { 0, 1, 0 }, { 0, 0 }));
	vertices.push_back(StaticVertex({ 1,-1, 0 }, { 0, 0, 1 }, { 0, 0 }));

	m_Device->GetResourceCache()->CreateBuffer(&m_VertexBuffer, Arc::GpuBufferDesc()
		.SetSize(vertices.size() * sizeof(StaticVertex))
		.AddBufferUsage(Arc::BufferUsage::VertexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::DeviceLocal));
	m_Device->UploadToDeviceLocalBuffer(&m_VertexBuffer, vertices.data(), vertices.size() * sizeof(StaticVertex));

	//m_Device->GetResourceCache()->CreateBuffer(&m_IndexBuffer, Arc::GpuBufferDesc()
	//	.SetSize(indices.size() * sizeof(uint32_t))
	//	.AddBufferUsage(Arc::BufferUsage::IndexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
	//	.AddMemoryPropertyFlag(Arc::MemoryProperty::DeviceLocal));
	//m_Device->UploadToDeviceLocalBuffer(&m_IndexBuffer, indices.data(), indices.size() * sizeof(uint32_t));

}

Renderer::~Renderer()
{
	//m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
	Arc::ImGuiShutdown();
	m_Device->GetResourceCache()->PrintHeapBudgets();
	m_Device->GetResourceCache()->FreeResources();
}

void Renderer::RenderFrame(FrameData& frameData)
{
	//m_RenderThreadFuture.wait();

	Arc::ImGuiBeginFrame();

	m_CameraFrameData.view = frameData.View;
	m_CameraFrameData.projection = frameData.Projection;

	ImGui::Begin("Profiling", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	auto res = m_Device->GetGpuProfiler()->GetResults();
	for (auto& el : res)
	{
		std::string s = el.taskName + ": " + std::to_string(el.timeInMs) + "ms";
		ImGui::Text(s.c_str());
	}
	ImGui::End();

	//m_RenderThreadFuture = std::async([&]() {

	Arc::FrameData fd = m_PresentQueue->BeginFrame();

	void* copyData = m_Device->GetResourceCache()->MapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);
	memcpy(copyData, &m_CameraFrameData, sizeof(CameraFrameData));
	m_Device->GetResourceCache()->UnmapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);

	fd.CommandBuffer->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex) });

	m_Device->GetRenderGraph()->SetRecordFunc(m_GeometryPassProxy,
		[=](Arc::CommandBuffer* cb, uint32_t frameIndex) {
			cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetHandle());
			//cb->BindVertexBuffers({ m_VertexBuffer.GetHandle() });
			//cb->Draw(3, 1, 0, 0);

			for (size_t i = 0; i < frameData.Models.size(); i++)
			{
				Mesh m = frameData.Models[i].Mesh;
				cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
				cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
				cb->PushConstants(m_MeshPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, 64, &frameData.Models[i].Transform);
				cb->DrawIndexed(m.IndexCount, 1, 0, 0, 0);
			}
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

	//	});
}

void Renderer::CompileShaders()
{
	m_MeshVertexShader = std::make_unique<Arc::Shader>();
	m_MeshFragmentShader = std::make_unique<Arc::Shader>();
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();
	SpirvHelper::Init();
	m_Device->GetResourceCache()->CreateShader(m_MeshVertexShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/Mesh.vert"));
	m_Device->GetResourceCache()->CreateShader(m_MeshFragmentShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/Mesh.frag"));
	m_Device->GetResourceCache()->CreateShader(m_PresentVertexShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/Present.vert"));
	m_Device->GetResourceCache()->CreateShader(m_PresentFragmentShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/Present.frag"));
	SpirvHelper::Finalize();
}

void Renderer::PreparePipelines()
{
	Arc::VertexAttributes meshAttributes(
		{
			{ Arc::Format::R32G32B32_Sfloat, offsetof(StaticVertex, Position) },
			{ Arc::Format::R32G32B32_Sfloat, offsetof(StaticVertex, Normal) },
			{ Arc::Format::R32G32_Sfloat, offsetof(StaticVertex, TexCoord) },
		}, sizeof(StaticVertex));

	Arc::VertexAttributes emptyAttributes({ }, 0);

	m_MeshPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_MeshPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_MeshVertexShader.get())
		.AddShaderStage(m_MeshFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_GeometryPassProxy))
		.SetVertexAttributes(meshAttributes));

	m_PresentPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_PresentPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_PresentVertexShader.get())
		.AddShaderStage(m_PresentFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleFan)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_PresentPassProxy))
		.SetVertexAttributes(emptyAttributes));

}

void Renderer::BuildRenderGraph()
{
	m_GeometryPassProxy = m_Device->GetRenderGraph()->AddPass("geometry", Arc::RenderPassDesc()
		.SetColorAttachments({
			{ m_ColorAttachment->GetProxy(), Arc::AttachmentLoadOp::Clear, Arc::AttachmentStoreOp::Store }
			})
		.SetDepthAttachment({ m_DepthAttachment->GetProxy(), Arc::AttachmentLoadOp::Clear, Arc::AttachmentStoreOp::DontCare })
		.SetExtent(m_PresentQueue->GetSwapchain()->GetExtent()));

	m_PresentPassProxy = m_Device->GetRenderGraph()->AddPass("present", Arc::PresentPassDesc()
		.SetInputImages({ m_ColorAttachment->GetProxy() })
		.SetFormat(m_PresentQueue->GetSwapchain()->GetSurfaceFormat().format)
		.SetImageViews(m_PresentQueue->GetSwapchain()->GetImageViews())
		.SetExtent(m_PresentQueue->GetSwapchain()->GetExtent()));

	m_Device->GetRenderGraph()->BuildGraph();

}

void Renderer::WaitForFrameEnd()
{
	//m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
}

void Renderer::RecompileShaders()
{

}

void Renderer::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
	m_WindowSize = m_PresentQueue->GetSwapchain()->GetExtent();

	m_Device->GetResourceCache()->ReleaseResource(m_ColorAttachment.get());
	m_Device->GetResourceCache()->ReleaseResource(m_DepthAttachment.get());

	m_Device->GetResourceCache()->CreateImage(m_ColorAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R16G16B16A16_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::ColorAttachment)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	m_Device->GetResourceCache()->CreateImage(m_DepthAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::D32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::DepthStencilAttachment));

	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ColorAttachment->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal));

	BuildRenderGraph();
}