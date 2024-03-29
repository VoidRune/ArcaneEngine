#include "Renderer.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include "Graphics/Vulkan/ShaderCompiler.h"
#include "Core/Log.h"
#include "Core/Timer.h"


Renderer* Renderer::s_Instance = nullptr;

Renderer::Renderer(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue)
{
	s_Instance = this;
	m_Window = window;
	m_Device = core;
	m_PresentQueue = presentQueue;
	Arc::ImGuiInit(window->GetHandle(), core, presentQueue);

	m_WindowSize = presentQueue->GetSwapchain()->GetExtent();

	CompileShaders();

	/* Prepare global data bound at the start of a frame */
	/* Camera */
	m_CameraFrameDataBuffer = std::make_unique<Arc::GpuBufferSet>();
	m_Device->GetResourceCache()->CreateBufferSet(m_CameraFrameDataBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(CameraFrameData))
		.AddBufferUsage(Arc::BufferUsage::UniformBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));

	m_TransformMatricesBuffer = std::make_unique<Arc::GpuBufferSet>();
	m_Device->GetResourceCache()->CreateBufferSet(m_TransformMatricesBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(glm::mat4) * m_MaxTransforms)
		.AddBufferUsage(Arc::BufferUsage::StorageBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));

	m_BoneMatricesBuffer = std::make_unique<Arc::GpuBufferSet>();
	m_Device->GetResourceCache()->CreateBufferSet(m_BoneMatricesBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(glm::mat4) * m_MaxBoneMatrices)
		.AddBufferUsage(Arc::BufferUsage::StorageBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));

	/* Corresponding descriptor */
	m_GlobalDescriptor = std::make_unique<Arc::DescriptorSetArray>();
	m_Device->GetResourceCache()->AllocateInFlightDescriptorSet(m_GlobalDescriptor.get(), Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::VertexFragment)
		.AddBinding(1, Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::Vertex)
		.AddBinding(1, Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::Vertex));
	m_Device->UpdateDescriptorSetArray(m_GlobalDescriptor.get(), Arc::DescriptorArrayWriteDesc()
		.AddBufferWrite(0, Arc::DescriptorType::UniformBuffer, m_CameraFrameDataBuffer->GetHandle(), 0, sizeof(CameraFrameData))
		.AddBufferWrite(1, Arc::DescriptorType::StorageBuffer, m_TransformMatricesBuffer->GetHandle(), 0, sizeof(glm::mat4) * m_MaxTransforms)
		.AddBufferWrite(2, Arc::DescriptorType::StorageBuffer, m_BoneMatricesBuffer->GetHandle(), 0, sizeof(glm::mat4) * m_MaxBoneMatrices));

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
	m_Device->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest));
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Linear)
		.SetMagFilter(Arc::Filter::Linear)
		.SetAddressMode(Arc::SamplerAddressMode::MirroredRepeat));

	m_BindlessTexturesDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_BindlessTexturesDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1024, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment)
		.AddFlag(Arc::DescriptorFlags::Bindless));

	m_PostprocessDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_PostprocessDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment));
	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ColorAttachment->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal));

	BuildRenderGraph();

	PreparePipelines();

	m_FrameRenderDataIndex = 0;
	m_FrameRenderData.resize(m_Device->GetImageCount());

	m_RenderThreadFuture = std::promise<void>().get_future();
}

Renderer::~Renderer()
{
	WaitForFrameEnd();
	Arc::ImGuiShutdown();
	m_Device->GetResourceCache()->PrintHeapBudgets();
	m_Device->GetResourceCache()->FreeResources();
}

void Renderer::RenderFrame()
{
	m_RenderThreadFuture.wait();

	RenderFrameData& frameData = m_FrameRenderData[m_FrameRenderDataIndex];
	m_FrameRenderDataIndex = (m_FrameRenderDataIndex + 1) % m_FrameRenderData.size();

	Arc::ImGuiBeginFrame();

	m_CameraFrameData.View = frameData.View;
	m_CameraFrameData.Projection = frameData.Projection;
	m_CameraFrameData.InvView = frameData.InvView;
	m_CameraFrameData.InvProjection = frameData.InvProjection;

	ImGui::Begin("Profiling", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	auto res = m_Device->GetGpuProfiler()->GetResults();
	for (auto& el : res)
	{
		std::string s = el.taskName + ": " + std::to_string(el.timeInMs) + "ms";
		ImGui::Text(s.c_str());
	}
	ImGui::End();

	m_RenderThreadFuture = std::async([&]() {

		Arc::FrameData fd = m_PresentQueue->BeginFrame();

		void* copyData = m_Device->GetResourceCache()->MapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);
		memcpy(copyData, &m_CameraFrameData, sizeof(CameraFrameData));
		m_Device->GetResourceCache()->UnmapMemory(m_CameraFrameDataBuffer.get(), fd.FrameIndex);

		copyData = m_Device->GetResourceCache()->MapMemory(m_TransformMatricesBuffer.get(), fd.FrameIndex);
		memcpy(copyData, frameData.Transformations.data(), frameData.Transformations.size() * sizeof(glm::mat4));
		m_Device->GetResourceCache()->UnmapMemory(m_TransformMatricesBuffer.get(), fd.FrameIndex);

		copyData = m_Device->GetResourceCache()->MapMemory(m_BoneMatricesBuffer.get(), fd.FrameIndex);
		memcpy(copyData, frameData.BoneMatrices.data(), frameData.BoneMatrices.size() * sizeof(glm::mat4));
		m_Device->GetResourceCache()->UnmapMemory(m_BoneMatricesBuffer.get(), fd.FrameIndex);


		fd.CommandBuffer->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex) });

		m_Device->GetRenderGraph()->SetRecordFunc(m_GeometryPassProxy,
			[=](Arc::CommandBuffer* cb, uint32_t frameIndex) {
				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetHandle());
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetLayout(), 1, { m_BindlessTexturesDescriptor->GetHandle() });

				for (auto& drawCall : frameData.DrawCalls)
				{
					if (drawCall.InstanceCount == 0)
						continue;
					Mesh m = drawCall.Model.Mesh;
					cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
					cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
					uint32_t imageIndices[2] = { drawCall.Model.BaseColorTexture.ArrayIndex, drawCall.Model.NormalTexture.ArrayIndex };
					cb->PushConstants(m_MeshPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, sizeof(imageIndices), &imageIndices);
					cb->DrawIndexed(m.IndexCount, drawCall.InstanceCount, 0, 0, drawCall.InstanceIndex);
				}

				Mesh m = frameData.AnimatedModel.Mesh;
				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_DynamicMeshPipeline->GetHandle());
				cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
				cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
				cb->DrawIndexed(m.IndexCount, 1, 0, 0, frameData.AnimatedInstanceIndex);

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

void Renderer::BindBindlessTexture(uint32_t arrayIndex, Arc::Image image)
{
	m_Device->UpdateDescriptorSet(m_BindlessTexturesDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_LinearSampler->GetHandle(), image.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal, arrayIndex));
}

void Renderer::CompileShaders()
{
	m_MeshVertexShader = std::make_unique<Arc::Shader>();
	m_MeshFragmentShader = std::make_unique<Arc::Shader>();
	m_DynamicMeshVertexShader = std::make_unique<Arc::Shader>();
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();

	Arc::ShaderCompiler::Initialize();
	Arc::ShaderDesc shaderDesc;
	Arc::ShaderCompiler::Compile("res/Shaders/Mesh.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_MeshVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Mesh.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_MeshFragmentShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/DynamicMesh.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_DynamicMeshVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Present.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_PresentVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Present.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_PresentFragmentShader.get(), shaderDesc);
	Arc::ShaderCompiler::Finalize();
}

void Renderer::PreparePipelines()
{
	Arc::VertexAttributes meshAttributes(
		{
			{ Arc::Format::R32G32B32_Sfloat, offsetof(StaticVertex, Position) },
			{ Arc::Format::R32G32B32_Sfloat, offsetof(StaticVertex, Normal) },
			{ Arc::Format::R32G32B32_Sfloat, offsetof(StaticVertex, Tangent) },
			{ Arc::Format::R32G32_Sfloat, offsetof(StaticVertex, TexCoord) },
		}, sizeof(StaticVertex));

	Arc::VertexAttributes dynamicMeshAttributes(
		{
			{ Arc::Format::R32G32B32_Sfloat, offsetof(DynamicVertex, Position) },
			{ Arc::Format::R32G32B32_Sfloat, offsetof(DynamicVertex, Normal) },
			{ Arc::Format::R32G32B32_Sfloat, offsetof(DynamicVertex, Tangent) },
			{ Arc::Format::R32G32_Sfloat, offsetof(DynamicVertex, TexCoord) },
			{ Arc::Format::R32G32B32A32_Sfloat, offsetof(DynamicVertex, BoneIndices) },
			{ Arc::Format::R32G32B32A32_Sfloat, offsetof(DynamicVertex, BoneWeights) },
		}, sizeof(DynamicVertex));

	Arc::VertexAttributes emptyAttributes({ }, 0);

	m_MeshPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_MeshPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_MeshVertexShader.get())
		.AddShaderStage(m_MeshFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_GeometryPassProxy))
		.SetVertexAttributes(meshAttributes));

	m_DynamicMeshPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_DynamicMeshPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_DynamicMeshVertexShader.get())
		.AddShaderStage(m_MeshFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_GeometryPassProxy))
		.SetVertexAttributes(dynamicMeshAttributes));

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
	m_RenderThreadFuture.wait();
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