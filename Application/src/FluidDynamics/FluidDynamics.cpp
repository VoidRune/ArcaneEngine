#include "FluidDynamics.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"

FluidDynamics::FluidDynamics(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();

	CreatePipelines();
	CreateSamplers();
	CreateImages();
}

void FluidDynamics::CreatePipelines()
{
	m_AddForcesShader = std::make_unique<Arc::Shader>();
	m_DiffusionShader = std::make_unique<Arc::Shader>();
	m_AdvectionShader = std::make_unique<Arc::Shader>();
	m_DivergenceShader = std::make_unique<Arc::Shader>();
	m_ProjectionShader = std::make_unique<Arc::Shader>();
	m_JacobiIterationShader = std::make_unique<Arc::Shader>();
	m_PresentVertShader = std::make_unique<Arc::Shader>();
	m_PresentFragShader = std::make_unique<Arc::Shader>();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/addForces.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_AddForcesShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/diffusion.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_DiffusionShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/advection.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_AdvectionShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/divergence.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_DivergenceShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/jacobiIteration.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_JacobiIterationShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/projection.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_ProjectionShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/shader.vert", shaderDesc))
		m_ResourceCache->CreateShader(m_PresentVertShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/FluidDynamics/shader.frag", shaderDesc))
		m_ResourceCache->CreateShader(m_PresentFragShader.get(), shaderDesc);

	m_PresentPipeline = std::make_unique<Arc::Pipeline>();;
	m_ResourceCache->CreatePipeline(m_PresentPipeline.get(), Arc::PipelineDesc{
		.ShaderStages = { m_PresentVertShader.get(), m_PresentFragShader.get() },
		.Topology = Arc::PrimitiveTopology::TriangleFan,
		.ColorAttachmentFormats = { m_PresentQueue->GetSurfaceFormat() },
		.UsePushDescriptors = true
	});

	m_AddForcesPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_AddForcesPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_AddForcesShader.get(),
		.UsePushDescriptors = true
		});

	m_DiffusionPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_DiffusionPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_DiffusionShader.get(),
		.UsePushDescriptors = true
		});

	m_AdvectionPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_AdvectionPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_AdvectionShader.get(),
		.UsePushDescriptors = true
		});

	m_DivergencePipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_DivergencePipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_DivergenceShader.get(),
		.UsePushDescriptors = true
		});

	m_JacobiIterationPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_JacobiIterationPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_JacobiIterationShader.get(),
		.UsePushDescriptors = true
		});

	m_ProjectionPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_ProjectionPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_ProjectionShader.get(),
		.UsePushDescriptors = true
		});
}

void FluidDynamics::CreateSamplers()
{
	m_NearestSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_NearestSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Nearest,
		.MagFilter = Arc::Filter::Nearest,
		.AddressMode = Arc::SamplerAddressMode::MirroredRepeat
		});
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Linear,
		.MagFilter = Arc::Filter::Linear,
		.AddressMode = Arc::SamplerAddressMode::MirroredRepeat
		});
}

void FluidDynamics::CreateImages()
{
	uint32_t w = m_PresentQueue->GetExtent()[0];
	uint32_t h = m_PresentQueue->GetExtent()[1];

	m_Device->WaitIdle();

	if (m_Velocity.get())
		m_ResourceCache->ReleaseResource(m_Velocity.get());

	m_Velocity = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_Velocity.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R16G16_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	m_Device->TransitionImageLayout(m_Velocity.get(), Arc::ImageLayout::General);

	if (m_DivergencePressure.get())
		m_ResourceCache->ReleaseResource(m_DivergencePressure.get());

	m_DivergencePressure = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_DivergencePressure.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R16G16_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	m_Device->TransitionImageLayout(m_DivergencePressure.get(), Arc::ImageLayout::General);

	if (m_Dye.get())
		m_ResourceCache->ReleaseResource(m_Dye.get());

	m_Dye = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_Dye.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	m_Device->TransitionImageLayout(m_Dye.get(), Arc::ImageLayout::General);

}


FluidDynamics::~FluidDynamics()
{

}

void FluidDynamics::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	lastTime = elapsedTime;

	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	glm::vec2 currentMousePos = { Arc::Input::GetMouseX(), Arc::Input::GetMouseY() };
	auto delta = currentMousePos - m_LastMousePos;
	m_LastMousePos = currentMousePos;

	fluidData.SourceColor = glm::vec4(HsvToRgb(elapsedTime * 0.1f, 1, 1), 1);
	fluidData.SourcePos = currentMousePos;
	fluidData.SourceVelocity = delta;
	fluidData.SourceRadius = Arc::Input::IsKeyDown(Arc::KeyCode::MouseLeft) ? 25.0f : 0.0f;
	fluidData.DeltaTime = dt * 100;


	m_RenderGraph->AddPass(Arc::RenderPass{
	.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
		cmd->BindComputePipeline(m_AddForcesPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_AddForcesPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_Dye->GetImageView(), Arc::ImageLayout::General, nullptr)));
		cmd->PushConstants(Arc::ShaderStage::Compute, m_AddForcesPipeline->GetLayout(), &fluidData, sizeof(fluidData));
		cmd->Dispatch(std::ceil(m_Velocity->GetExtent()[0] / 32.0f), std::ceil(m_Velocity->GetExtent()[1] / 32.0f), 1);

		cmd->BindComputePipeline(m_AdvectionPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_AdvectionPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::CombinedImageSampler, m_Velocity->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle()))
			.AddWrite(Arc::PushImageWrite(2, Arc::DescriptorType::StorageImage, m_Dye->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle()))
			.AddWrite(Arc::PushImageWrite(3, Arc::DescriptorType::CombinedImageSampler, m_Dye->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle())));
		cmd->Dispatch(std::ceil(m_Velocity->GetExtent()[0] / 32.0f), std::ceil(m_Velocity->GetExtent()[1] / 32.0f), 1);

		cmd->BindComputePipeline(m_DiffusionPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_DiffusionPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_Dye->GetImageView(), Arc::ImageLayout::General, nullptr)));
		for (size_t i = 0; i < 100; i++)
		{
			cmd->Dispatch(std::ceil(m_Velocity->GetExtent()[0] / 32.0f), std::ceil(m_Velocity->GetExtent()[1] / 32.0f), 1);
		}

		cmd->BindComputePipeline(m_DivergencePipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_DivergencePipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_DivergencePressure->GetImageView(), Arc::ImageLayout::General, nullptr)));
		cmd->Dispatch(std::ceil(m_DivergencePressure->GetExtent()[0] / 32.0f), std::ceil(m_DivergencePressure->GetExtent()[1] / 32.0f), 1);

		cmd->BindComputePipeline(m_JacobiIterationPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_JacobiIterationPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_DivergencePressure->GetImageView(), Arc::ImageLayout::General, nullptr)));
		for (size_t i = 0; i < 250; i++)
		{
			cmd->Dispatch(std::ceil(m_DivergencePressure->GetExtent()[0] / 32.0f), std::ceil(m_DivergencePressure->GetExtent()[1] / 32.0f), 1);
		}

		cmd->BindComputePipeline(m_ProjectionPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_ProjectionPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_DivergencePressure->GetImageView(), Arc::ImageLayout::General, nullptr)));
		cmd->Dispatch(std::ceil(m_Velocity->GetExtent()[0] / 32.0f), std::ceil(m_Velocity->GetExtent()[1] / 32.0f), 1);
	}});

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
	.LoadOp = Arc::AttachmentLoadOp::Clear,
	.ClearColor = {0.1, 0.6, 0.6, 1.0},
	.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
		cmd->BindPipeline(m_PresentPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::CombinedImageSampler, m_Dye->GetImageView(), Arc::ImageLayout::General, m_NearestSampler->GetHandle())));
		cmd->Draw(6, 1, 0, 0);
	}});

	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
	m_Device->WaitIdle();
}

glm::vec3 FluidDynamics::HsvToRgb(float h, float s, float v)
{
	h = std::fmod(h, 1.0f);
	float c = v * s;
	float x = c * (1 - std::fabs(fmod(h * 6.0f, 2.0f) - 1));
	float m = v - c;

	glm::vec3 rgb;

	if (h < 1.0f / 6.0f)       rgb = glm::vec3(c, x, 0);
	else if (h < 2.0f / 6.0f)  rgb = glm::vec3(x, c, 0);
	else if (h < 3.0f / 6.0f)  rgb = glm::vec3(0, c, x);
	else if (h < 4.0f / 6.0f)  rgb = glm::vec3(0, x, c);
	else if (h < 5.0f / 6.0f)  rgb = glm::vec3(x, 0, c);
	else                      rgb = glm::vec3(c, 0, x);

	return rgb + glm::vec3(m);
}

void FluidDynamics::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
	CreateImages();
}

void FluidDynamics::RecompileShaders()
{

}

void FluidDynamics::WaitForFrameEnd()
{

}