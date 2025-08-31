#include "FluidDynamics.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"
#include "Utility.h"
#include "stb/stb_image_write.h"

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
	uint32_t w = m_PresentQueue->GetExtent()[0] * m_Resolution;
	uint32_t h = m_PresentQueue->GetExtent()[1] * m_Resolution;
	m_CanvasSize = { w, h };
	m_ThreadDispatchSize = glm::ceil(glm::vec2(m_CanvasSize) / 32.0f);
	float clearColor[4] = { 0, 0, 0, 0 };

	m_Device->WaitIdle();

	if (m_Velocity1.get())
		m_ResourceCache->ReleaseResource(m_Velocity1.get());
	if (m_Velocity2.get())
		m_ResourceCache->ReleaseResource(m_Velocity2.get());

	m_Velocity1 = std::make_unique<Arc::GpuImage>();
	m_Velocity2 = std::make_unique<Arc::GpuImage>();
	auto velocityDesc = Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R16G16_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
	};
	m_ResourceCache->CreateGpuImage(m_Velocity1.get(), velocityDesc);
	m_ResourceCache->CreateGpuImage(m_Velocity2.get(), velocityDesc);
	m_Device->TransitionImageLayout(m_Velocity1.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_Velocity2.get(), Arc::ImageLayout::General);
	m_Device->ClearColorImage(m_Velocity1.get(), clearColor, Arc::ImageLayout::General);
	m_Device->ClearColorImage(m_Velocity2.get(), clearColor, Arc::ImageLayout::General);

	if (m_DivergencePressure1.get())
		m_ResourceCache->ReleaseResource(m_DivergencePressure1.get());
	if (m_DivergencePressure2.get())
		m_ResourceCache->ReleaseResource(m_DivergencePressure2.get());

	m_DivergencePressure1 = std::make_unique<Arc::GpuImage>();
	m_DivergencePressure2 = std::make_unique<Arc::GpuImage>();
	auto diverPressureDesc = Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R16G16_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
	};
	m_ResourceCache->CreateGpuImage(m_DivergencePressure1.get(), diverPressureDesc);
	m_ResourceCache->CreateGpuImage(m_DivergencePressure2.get(), diverPressureDesc);
	m_Device->TransitionImageLayout(m_DivergencePressure1.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_DivergencePressure2.get(), Arc::ImageLayout::General);

	if (m_Dye1.get())
		m_ResourceCache->ReleaseResource(m_Dye1.get());
	if (m_Dye2.get())
		m_ResourceCache->ReleaseResource(m_Dye2.get());

	m_Dye1 = std::make_unique<Arc::GpuImage>();
	m_Dye2 = std::make_unique<Arc::GpuImage>();
	auto dyeDesc = Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R16G16B16A16_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
	};
	m_ResourceCache->CreateGpuImage(m_Dye1.get(), dyeDesc);
	m_ResourceCache->CreateGpuImage(m_Dye2.get(), dyeDesc);
	m_Device->TransitionImageLayout(m_Dye1.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_Dye2.get(), Arc::ImageLayout::General);
	m_Device->ClearColorImage(m_Dye1.get(), clearColor, Arc::ImageLayout::General);
	m_Device->ClearColorImage(m_Dye2.get(), clearColor, Arc::ImageLayout::General);
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

	glm::vec2 currentMousePos = { Arc::Input::GetMouseX() * m_Resolution, Arc::Input::GetMouseY() * m_Resolution };
	auto delta = currentMousePos - m_LastMousePos;
	m_LastMousePos = currentMousePos;

	fluidData.SourceColor = glm::vec4(HsvToRgb(elapsedTime * 0.1f, 1, 1), 1);
	fluidData.SourcePos = currentMousePos;
	fluidData.SourceVelocity = delta;
	fluidData.SourceRadius = Arc::Input::IsKeyDown(Arc::KeyCode::MouseLeft) ? 50.0f * m_Resolution : 0.0f;
	fluidData.DeltaTime = dt * 50;

	if (Arc::Input::IsKeyPressed(Arc::KeyCode::G))
	{
		std::vector<uint8_t> imageData = m_Device->GetImageData(m_Dye1.get());
		std::string path = "img.png";
		stbi_write_png(path.c_str(), m_CanvasSize.x, m_CanvasSize.y, 4, imageData.data(), m_CanvasSize.x * 4);
	}

	m_RenderGraph->AddPass(Arc::RenderPass{
	.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
		cmd->BindComputePipeline(m_AddForcesPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_AddForcesPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity1->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_Dye1->GetImageView(), Arc::ImageLayout::General, nullptr)));
		cmd->PushConstants(Arc::ShaderStage::Compute, m_AddForcesPipeline->GetLayout(), &fluidData, sizeof(fluidData));
		cmd->Dispatch(m_ThreadDispatchSize.x, m_ThreadDispatchSize.y, 1);

		cmd->BindComputePipeline(m_AdvectionPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_AdvectionPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity2->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::CombinedImageSampler, m_Velocity1->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle()))
			.AddWrite(Arc::PushImageWrite(2, Arc::DescriptorType::StorageImage, m_Dye2->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle()))
			.AddWrite(Arc::PushImageWrite(3, Arc::DescriptorType::CombinedImageSampler, m_Dye1->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle())));
		cmd->Dispatch(m_ThreadDispatchSize.x, m_ThreadDispatchSize.y, 1);
		std::swap(m_Velocity1, m_Velocity2);
		std::swap(m_Dye1, m_Dye2);

		//cmd->BindComputePipeline(m_DiffusionPipeline->GetHandle());
		//for (size_t i = 0; i < 100; i++)
		//{
		//	cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_DiffusionPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
		//		.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity1->GetImageView(), Arc::ImageLayout::General, nullptr))
		//		.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_Dye1->GetImageView(), Arc::ImageLayout::General, nullptr))
		//		.AddWrite(Arc::PushImageWrite(2, Arc::DescriptorType::StorageImage, m_Velocity2->GetImageView(), Arc::ImageLayout::General, nullptr))
		//		.AddWrite(Arc::PushImageWrite(3, Arc::DescriptorType::StorageImage, m_Dye2->GetImageView(), Arc::ImageLayout::General, nullptr)));
		//	cmd->Dispatch(m_ThreadDispatchSize.x, m_ThreadDispatchSize.y, 1);
		//	std::swap(m_Velocity1, m_Velocity2);
		//	std::swap(m_Dye1, m_Dye2);
		//}

		cmd->BindComputePipeline(m_DivergencePipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_DivergencePipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity1->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_DivergencePressure1->GetImageView(), Arc::ImageLayout::General, nullptr)));
		cmd->Dispatch(m_ThreadDispatchSize.x, m_ThreadDispatchSize.y, 1);

		cmd->BindComputePipeline(m_JacobiIterationPipeline->GetHandle());
		for (size_t i = 0; i < 250; i++)
		{
			cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_JacobiIterationPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
				.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_DivergencePressure1->GetImageView(), Arc::ImageLayout::General, nullptr))
				.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_DivergencePressure2->GetImageView(), Arc::ImageLayout::General, nullptr)));
			cmd->Dispatch(m_ThreadDispatchSize.x, m_ThreadDispatchSize.y, 1);
			std::swap(m_DivergencePressure1, m_DivergencePressure2);
		}

		cmd->BindComputePipeline(m_ProjectionPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_ProjectionPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_Velocity1->GetImageView(), Arc::ImageLayout::General, nullptr))
			.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_DivergencePressure1->GetImageView(), Arc::ImageLayout::General, nullptr)));
		cmd->Dispatch(m_ThreadDispatchSize.x, m_ThreadDispatchSize.y, 1);
	}});

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
	.LoadOp = Arc::AttachmentLoadOp::Clear,
	.ClearColor = {0.1, 0.6, 0.6, 1.0},
	.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
		cmd->BindPipeline(m_PresentPipeline->GetHandle());
		cmd->PushDescriptorSets(Arc::PipelineBindPoint::Graphics, m_PresentPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
			.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::CombinedImageSampler, m_Dye1->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle())));
		cmd->Draw(6, 1, 0, 0);
	}});

	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
	m_Device->WaitIdle();
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