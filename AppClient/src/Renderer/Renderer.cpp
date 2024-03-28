#include "Renderer.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include "Graphics/Vulkan/ShaderCompiler.h"
#include "Core/Log.h"
#include "Core/Timer.h"


Arc::Window* m_Window;
Arc::Device* m_Device;
Arc::PresentQueue* m_PresentQueue;
VkExtent2D m_WindowSize;
std::future<void> m_RenderThreadFuture{};

uint32_t m_FrameRenderDataIndex;
std::vector<RenderFrameData> m_FrameRenderData;

/* Per frame descriptor */
struct CameraFrameData
{
	glm::mat4 View;
	glm::mat4 Projection;
	glm::mat4 InvView;
	glm::mat4 InvProjection;
	glm::mat4 OrthoProjection;
	glm::mat4 ShadowmapViewProj;
} m_CameraFrameData;

std::unique_ptr<Arc::GpuBufferSet> m_CameraFrameDataBuffer;
std::unique_ptr<Arc::DescriptorSetArray> m_GlobalDescriptor;
std::unique_ptr<Arc::DescriptorSet> m_BindlessTexturesDescriptor;
std::vector<uint32_t> m_TextureBindingIndices;

uint32_t m_MaxTransforms = 4096;
std::unique_ptr<Arc::GpuBufferSet> m_TransformMatricesBuffer;
uint32_t m_MaxBoneMatrices = 4096;
std::unique_ptr<Arc::GpuBufferSet> m_BoneMatricesBuffer;
std::unique_ptr<Arc::DescriptorSetArray> m_ShadowMapDescriptor;
std::unique_ptr<Arc::DescriptorSetArray> m_TransformDescriptor;

uint32_t m_MaxGuiVertices = 4096;
std::unique_ptr<Arc::GpuBufferSet> m_GuiVerticesBuffer;
std::unique_ptr<Arc::GpuBufferSet> m_GuiFontVerticesBuffer;

std::unique_ptr<Arc::Sampler> m_ShadowSampler;
std::unique_ptr<Arc::Sampler> m_PointSampler;
std::unique_ptr<Arc::Sampler> m_LinearSampler;
std::unique_ptr<Arc::DescriptorSet> m_PostprocessDescriptor;

/* Shaders */
std::unique_ptr<Arc::Shader> m_ShadowDepthShader;
std::unique_ptr<Arc::Shader> m_ShadowDepthDynamicShader;
std::unique_ptr<Arc::Shader> m_MeshVertexShader;
std::unique_ptr<Arc::Shader> m_MeshFragmentShader;
std::unique_ptr<Arc::Shader> m_DynamicMeshVertexShader;
std::unique_ptr<Arc::Shader> m_GuiVertexShader;
std::unique_ptr<Arc::Shader> m_GuiFragmentShader;
std::unique_ptr<Arc::Shader> m_FontFragmentShader;
std::unique_ptr<Arc::Shader> m_PresentVertexShader;
std::unique_ptr<Arc::Shader> m_PresentFragmentShader;

/* Pipelines */
std::unique_ptr<Arc::Pipeline> m_ShadowPipeline;
std::unique_ptr<Arc::Pipeline> m_ShadowDynamicPipeline;
std::unique_ptr<Arc::Pipeline> m_MeshPipeline;
std::unique_ptr<Arc::Pipeline> m_DynamicMeshPipeline;
std::unique_ptr<Arc::Pipeline> m_PresentPipeline;
std::unique_ptr<Arc::Pipeline> m_GuiPipeline;
std::unique_ptr<Arc::Pipeline> m_FontPipeline;

/* Render pass data*/
VkExtent2D m_ShadowExtent = { 1024, 1024 };
std::unique_ptr<Arc::Image> m_ShadowDepthAttachment;
std::unique_ptr<Arc::Image> m_ColorAttachment;
std::unique_ptr<Arc::Image> m_DepthAttachment;
Arc::RenderPassProxy m_ShadowPassProxy;
Arc::RenderPassProxy m_GeometryPassProxy;
Arc::PresentPassProxy m_PresentPassProxy;

void Renderer::Initialize(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = core;
	m_PresentQueue = presentQueue;
	Arc::ImGuiInit(window->GetHandle(), core, presentQueue);

	m_WindowSize = presentQueue->GetSwapchain()->GetExtent();

	CompileShaders();

	/* Buffers */
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

	m_GuiVerticesBuffer = std::make_unique<Arc::GpuBufferSet>();
	m_Device->GetResourceCache()->CreateBufferSet(m_GuiVerticesBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(GuiVertex) * m_MaxGuiVertices)
		.AddBufferUsage(Arc::BufferUsage::VertexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));

	m_GuiFontVerticesBuffer = std::make_unique<Arc::GpuBufferSet>();
	m_Device->GetResourceCache()->CreateBufferSet(m_GuiFontVerticesBuffer.get(), Arc::GpuBufferDesc()
		.SetSize(sizeof(GuiVertex) * m_MaxGuiVertices)
		.AddBufferUsage(Arc::BufferUsage::VertexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::HostVisible));

	/* Descriptors */
	m_GlobalDescriptor = std::make_unique<Arc::DescriptorSetArray>();
	m_Device->GetResourceCache()->AllocateDescriptorSetArray(m_GlobalDescriptor.get(), Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::VertexFragment));
	m_Device->UpdateDescriptorSetArray(m_GlobalDescriptor.get(), Arc::DescriptorArrayWriteDesc()
		.AddBufferWrite(0, m_CameraFrameDataBuffer->GetHandle(), 0, sizeof(CameraFrameData)));

	m_ShadowDepthAttachment = std::make_unique<Arc::Image>();
	m_ColorAttachment = std::make_unique<Arc::Image>();
	m_DepthAttachment = std::make_unique<Arc::Image>();

	m_Device->GetResourceCache()->CreateImage(m_ShadowDepthAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_ShadowExtent)
		.SetFormat(Arc::Format::D16_Unorm)
		.AddUsageFlag(Arc::ImageUsage::DepthStencilAttachment)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	m_Device->GetResourceCache()->CreateImage(m_ColorAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::R16G16B16A16_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::ColorAttachment)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	m_Device->GetResourceCache()->CreateImage(m_DepthAttachment.get(), Arc::ImageDesc()
		.SetExtent(m_WindowSize)
		.SetFormat(Arc::Format::D32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::DepthStencilAttachment));

	
	m_ShadowSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_ShadowSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Linear)
		.SetMagFilter(Arc::Filter::Linear)
		.SetAddressMode(Arc::SamplerAddressMode::ClampToBorder));
	m_PointSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_PointSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Nearest)
		.SetMagFilter(Arc::Filter::Nearest));
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_Device->GetResourceCache()->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc()
		.SetMinFilter(Arc::Filter::Linear)
		.SetMagFilter(Arc::Filter::Linear)
		.SetAddressMode(Arc::SamplerAddressMode::MirroredRepeat));

	uint32_t maxBindlessTextures = 1024;
	m_BindlessTexturesDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_BindlessTexturesDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(maxBindlessTextures, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment)
		.AddFlag(Arc::DescriptorFlags::Bindless));

	for (size_t i = 0; i < maxBindlessTextures; i++)
	{
		m_TextureBindingIndices.push_back(maxBindlessTextures - i - 1);
	}

	
	m_ShadowMapDescriptor = std::make_unique<Arc::DescriptorSetArray>();
	m_Device->GetResourceCache()->AllocateDescriptorSetArray(m_ShadowMapDescriptor.get(), Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::Vertex)
		.AddBinding(1, Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::Vertex));
	m_Device->UpdateDescriptorSetArray(m_ShadowMapDescriptor.get(), Arc::DescriptorArrayWriteDesc()
		.AddBufferWrite(0, m_TransformMatricesBuffer->GetHandle(), 0, sizeof(glm::mat4) * m_MaxTransforms)
		.AddBufferWrite(1, m_BoneMatricesBuffer->GetHandle(), 0, sizeof(glm::mat4) * m_MaxBoneMatrices));

	m_TransformDescriptor = std::make_unique<Arc::DescriptorSetArray>();
	m_Device->GetResourceCache()->AllocateDescriptorSetArray(m_TransformDescriptor.get(), Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::Vertex)
		.AddBinding(1, Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::Vertex)
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment));
	m_Device->UpdateDescriptorSetArray(m_TransformDescriptor.get(), Arc::DescriptorArrayWriteDesc()
		.AddBufferWrite(0, m_TransformMatricesBuffer->GetHandle(), 0, sizeof(glm::mat4) * m_MaxTransforms)
		.AddBufferWrite(1, m_BoneMatricesBuffer->GetHandle(), 0, sizeof(glm::mat4) * m_MaxBoneMatrices)
		.AddImageWrite(2, m_ShadowSampler->GetHandle(), m_ShadowDepthAttachment->GetImageView(), Arc::ImageLayout::DepthReadOnlyOptimal));

	m_PostprocessDescriptor = std::make_unique<Arc::DescriptorSet>();
	m_Device->GetResourceCache()->AllocateDescriptorSets({ m_PostprocessDescriptor.get() }, Arc::DescriptorSetLayoutDesc()
		.AddBinding(1, Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::Fragment));
	m_Device->UpdateDescriptorSet(m_PostprocessDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_PointSampler->GetHandle(), m_ColorAttachment->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal));
		//.AddImageWrite(0, m_PointSampler->GetHandle(), m_ShadowDepthAttachment->GetImageView(), Arc::ImageLayout::DepthReadOnlyOptimal));

	BuildRenderGraph();

	PreparePipelines();

	m_FrameRenderDataIndex = 0;
	m_FrameRenderData.resize(m_Device->GetImageCount());

	m_RenderThreadFuture = std::promise<void>().get_future();
}

void Renderer::Shutdown()
{
	WaitForFrameEnd();
	Arc::ImGuiShutdown();
	m_Device->GetResourceCache()->PrintHeapBudgets();
	m_Device->GetResourceCache()->FreeResources();
}

RenderFrameData& Renderer::GetFrameRenderData()
{
	return m_FrameRenderData[m_FrameRenderDataIndex];
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
	m_CameraFrameData.ShadowmapViewProj = frameData.ShadowViewProj;
	m_CameraFrameData.OrthoProjection = frameData.OrthoProjection;

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

		if (frameData.Transformations.size() > 0)
		{
			copyData = m_Device->GetResourceCache()->MapMemory(m_TransformMatricesBuffer.get(), fd.FrameIndex);
			memcpy(copyData, frameData.Transformations.data(), frameData.Transformations.size() * sizeof(glm::mat4));
			m_Device->GetResourceCache()->UnmapMemory(m_TransformMatricesBuffer.get(), fd.FrameIndex);
		}
		if (frameData.BoneMatrices.size() > 0)
		{
			copyData = m_Device->GetResourceCache()->MapMemory(m_BoneMatricesBuffer.get(), fd.FrameIndex);
			memcpy(copyData, frameData.BoneMatrices.data(), frameData.BoneMatrices.size() * sizeof(glm::mat4));
			m_Device->GetResourceCache()->UnmapMemory(m_BoneMatricesBuffer.get(), fd.FrameIndex);
		}
		if (frameData.GuiVertexData.size() > 0)
		{
			copyData = m_Device->GetResourceCache()->MapMemory(m_GuiVerticesBuffer.get(), fd.FrameIndex);
			memcpy(copyData, frameData.GuiVertexData.data(), frameData.GuiVertexData.size() * sizeof(GuiVertex));
			m_Device->GetResourceCache()->UnmapMemory(m_GuiVerticesBuffer.get(), fd.FrameIndex);
		}
		if (frameData.GuiFontVertexData.size() > 0)
		{
			copyData = m_Device->GetResourceCache()->MapMemory(m_GuiFontVerticesBuffer.get(), fd.FrameIndex);
			memcpy(copyData, frameData.GuiFontVertexData.data(), frameData.GuiFontVertexData.size() * sizeof(GuiVertex));
			m_Device->GetResourceCache()->UnmapMemory(m_GuiFontVerticesBuffer.get(), fd.FrameIndex);
		}

		fd.CommandBuffer->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex) });

		m_Device->GetRenderGraph()->SetRecordFunc(m_ShadowPassProxy,
			[&](Arc::CommandBuffer* cb, uint32_t frameIndex) {
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_ShadowPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex), m_ShadowMapDescriptor->GetHandle(fd.FrameIndex) });
				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_ShadowPipeline->GetHandle());
				for (auto& drawCall : frameData.StaticDrawCalls)
				{
					if (drawCall.InstanceCount == 0)
						continue;
					Mesh m = drawCall.Model.Mesh;
					cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
					cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
					cb->DrawIndexed(m.IndexCount, drawCall.InstanceCount, 0, 0, drawCall.InstanceIndex);
				}
				
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_ShadowDynamicPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex), m_ShadowMapDescriptor->GetHandle(fd.FrameIndex) });
				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_ShadowDynamicPipeline->GetHandle());
				for (auto& drawCall : frameData.DynamicDrawCalls)
				{
					if (drawCall.InstanceCount == 0)
						continue;
					Mesh m = drawCall.Model.Mesh;
					cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
					cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
					cb->PushConstants(m_MeshPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, sizeof(drawCall.BoneDataStart), &drawCall.BoneDataStart);
					cb->DrawIndexed(m.IndexCount, drawCall.InstanceCount, 0, 0, drawCall.InstanceIndex);
				}
				
			});

		m_Device->GetRenderGraph()->SetRecordFunc(m_GeometryPassProxy,
			[&](Arc::CommandBuffer* cb, uint32_t frameIndex) {
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex), m_TransformDescriptor->GetHandle(fd.FrameIndex), m_BindlessTexturesDescriptor->GetHandle() });

				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_MeshPipeline->GetHandle());
				for (auto& drawCall : frameData.StaticDrawCalls)
				{
					if (drawCall.InstanceCount == 0)
						continue;
					Mesh m = drawCall.Model.Mesh;
					cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
					cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
					uint32_t imageIndices[2] = { drawCall.Model.BaseColorTexture.TextureBinding, drawCall.Model.NormalTexture.TextureBinding };
					cb->PushConstants(m_MeshPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, sizeof(imageIndices), &imageIndices);
					cb->DrawIndexed(m.IndexCount, drawCall.InstanceCount, 0, 0, drawCall.InstanceIndex);
				}

				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_DynamicMeshPipeline->GetHandle());
				for (auto& drawCall : frameData.DynamicDrawCalls)
				{
					if (drawCall.InstanceCount == 0)
						continue;
					Mesh m = drawCall.Model.Mesh;
					cb->BindVertexBuffers({ m.VertexBuffer.GetHandle() });
					cb->BindIndexBuffer(m.IndexBuffer.GetHandle(), Arc::IndexType::Uint32);
					uint32_t imageIndices[3] = { drawCall.Model.BaseColorTexture.TextureBinding, drawCall.Model.NormalTexture.TextureBinding, drawCall.BoneDataStart };
					cb->PushConstants(m_MeshPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, sizeof(imageIndices), &imageIndices);
					cb->DrawIndexed(m.IndexCount, drawCall.InstanceCount, 0, 0, drawCall.InstanceIndex);
				}
			});

		m_Device->GetRenderGraph()->SetRecordFunc(m_PresentPassProxy,
			[&](Arc::CommandBuffer* cb, uint32_t frameIndex) {
				cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetHandle());
				cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, { m_PostprocessDescriptor->GetHandle() });
				cb->Draw(6, 1, 0, 0);

				if (frameData.GuiVertexData.size() > 0)
				{
					cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_GuiPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex), m_BindlessTexturesDescriptor->GetHandle() });
					cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_GuiPipeline->GetHandle());
					cb->BindVertexBuffers({ m_GuiVerticesBuffer->GetHandle()[fd.FrameIndex]});
					uint32_t imageIndex = frameData.FontTextureBinding;
					cb->PushConstants(m_GuiPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, sizeof(imageIndex), &imageIndex);
					cb->Draw(frameData.GuiVertexData.size(), 1, 0, 0);
				}
				if (frameData.GuiFontVertexData.size() > 0)
				{
					cb->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, m_FontPipeline->GetLayout(), 0, { m_GlobalDescriptor->GetHandle(fd.FrameIndex), m_BindlessTexturesDescriptor->GetHandle() });
					cb->BindPipeline(Arc::PipelineBindPoint::Graphics, m_FontPipeline->GetHandle());
					cb->BindVertexBuffers({ m_GuiFontVerticesBuffer->GetHandle()[fd.FrameIndex] });
					uint32_t imageIndex = frameData.FontTextureBinding;
					cb->PushConstants(m_FontPipeline->GetLayout(), Arc::ShaderStage::VertexFragment, sizeof(imageIndex), &imageIndex);
					cb->Draw(frameData.GuiFontVertexData.size(), 1, 0, 0);
				}

				Arc::ImGuiEndFrame(cb->GetHandle());
			});

		m_Device->GetRenderGraph()->Execute(fd, m_PresentQueue->GetSwapchain()->GetExtent());
		m_PresentQueue->EndFrame();

		});
}

uint32_t Renderer::BindTexture(Arc::Image image)
{
	uint32_t freeIndex = m_TextureBindingIndices.back();
	m_TextureBindingIndices.pop_back();

	m_Device->UpdateDescriptorSet(m_BindlessTexturesDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_LinearSampler->GetHandle(), image.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal, freeIndex));

	return freeIndex;
}

void Renderer::ReleaseTextureBinding(uint32_t binding)
{
	m_TextureBindingIndices.push_back(binding);
}

void Renderer::CompileShaders()
{
	m_ShadowDepthShader = std::make_unique<Arc::Shader>();
	m_ShadowDepthDynamicShader = std::make_unique<Arc::Shader>();
	m_MeshVertexShader = std::make_unique<Arc::Shader>();
	m_MeshFragmentShader = std::make_unique<Arc::Shader>();
	m_DynamicMeshVertexShader = std::make_unique<Arc::Shader>();
	m_GuiVertexShader = std::make_unique<Arc::Shader>();
	m_GuiFragmentShader = std::make_unique<Arc::Shader>();
	m_FontFragmentShader = std::make_unique<Arc::Shader>();
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();

	Arc::ShaderCompiler::Initialize();
	Arc::ShaderDesc shaderDesc;
	Arc::ShaderCompiler::Compile("res/Shaders/ShadowDepth.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_ShadowDepthShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/ShadowDepthDynamic.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_ShadowDepthDynamicShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Mesh.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_MeshVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Mesh.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_MeshFragmentShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/DynamicMesh.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_DynamicMeshVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Gui.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_GuiVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/Gui.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_GuiFragmentShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/GuiFont.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_FontFragmentShader.get(), shaderDesc);
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

	Arc::VertexAttributes guiMeshAttributes(
		{
			{ Arc::Format::R32G32_Sfloat, offsetof(GuiVertex, Position) },
			{ Arc::Format::R32G32_Sfloat, offsetof(GuiVertex, TexCoord) },
			{ Arc::Format::R32G32B32A32_Sfloat, offsetof(GuiVertex, Color) },
		}, sizeof(GuiVertex));

	Arc::VertexAttributes emptyAttributes({ }, 0);

	m_ShadowPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_ShadowPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_ShadowDepthShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		//.SetCullMode(Arc::CullMode::Front)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_ShadowPassProxy))
		.SetVertexAttributes(meshAttributes));

	m_ShadowDynamicPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_ShadowDynamicPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_ShadowDepthDynamicShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		//.SetCullMode(Arc::CullMode::Front)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_ShadowPassProxy))
		.SetVertexAttributes(dynamicMeshAttributes));


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

	m_GuiPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_GuiPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_GuiVertexShader.get())
		.AddShaderStage(m_GuiFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_PresentPassProxy))
		.SetVertexAttributes(guiMeshAttributes));

	m_FontPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_FontPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_GuiVertexShader.get())
		.AddShaderStage(m_FontFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleList)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_PresentPassProxy))
		.SetVertexAttributes(guiMeshAttributes));

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
	m_ShadowPassProxy = m_Device->GetRenderGraph()->AddPass("shadow", Arc::RenderPassDesc()
		.SetDepthAttachment({ m_ShadowDepthAttachment->GetProxy(), Arc::AttachmentLoadOp::Clear, Arc::AttachmentStoreOp::Store })
		.SetExtent(m_ShadowExtent));

	m_GeometryPassProxy = m_Device->GetRenderGraph()->AddPass("geometry", Arc::RenderPassDesc()
		.SetInputImages({ m_ShadowDepthAttachment->GetProxy() })
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