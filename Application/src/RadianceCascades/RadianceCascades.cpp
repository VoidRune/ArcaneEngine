#include "RadianceCascades.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"
#include "ArcaneEngine/Core/Timer.h"
#include "ArcaneEngine/Core/Log.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"

RadianceCascades::RadianceCascades(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();

	CreatePipelines();
	CreateImages();
}


void RadianceCascades::CreatePipelines()
{
	m_RadianceCascadesShader = std::make_unique<Arc::Shader>();
	m_JFAShader = std::make_unique<Arc::Shader>();
	m_CompositeVertShader = std::make_unique<Arc::Shader>();
	m_CompositeFragShader = std::make_unique<Arc::Shader>();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/radianceCascades.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_RadianceCascadesShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/jumpFloodAlgorithm.comp", shaderDesc))
		m_ResourceCache->CreateShader(m_JFAShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/shader.vert", shaderDesc))
		m_ResourceCache->CreateShader(m_CompositeVertShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/RadianceCascades/shader.frag", shaderDesc))
		m_ResourceCache->CreateShader(m_CompositeFragShader.get(), shaderDesc);

	m_RadianceCascadesPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_RadianceCascadesPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_RadianceCascadesShader.get(),
		.UsePushDescriptors = true
		});

	m_JFAPipeline = std::make_unique<Arc::ComputePipeline>();
	m_ResourceCache->CreateComputePipeline(m_JFAPipeline.get(), Arc::ComputePipelineDesc{
		.Shader = m_JFAShader.get(),
		.UsePushDescriptors = true
		});

	m_CompositePipeline = std::make_unique<Arc::Pipeline>();;
	m_ResourceCache->CreatePipeline(m_CompositePipeline.get(), Arc::PipelineDesc{
		.ShaderStages = { m_CompositeVertShader.get(), m_CompositeFragShader.get() },
		.Topology = Arc::PrimitiveTopology::TriangleFan,
		.ColorAttachmentFormats = { m_PresentQueue->GetSurfaceFormat() },
		.UsePushDescriptors = true
		});
}

void RadianceCascades::CreateImages()
{
	m_NearestSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_NearestSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Nearest,
		.MagFilter = Arc::Filter::Nearest,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Linear,
		.MagFilter = Arc::Filter::Linear,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});

	uint32_t w = m_PresentQueue->GetExtent()[0];
	uint32_t h = m_PresentQueue->GetExtent()[1];

	m_SeedImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_SeedImage.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	int x, y, c;
	const char* file = "res/RadianceCascadesCanvas/img2.png";
	stbi_info(file, &x, &y, &c);
	if (x == w && y == h)
	{
		uint8_t* data = stbi_load(file, &x, &y, &c, 4);
		m_Device->SetImageData(m_SeedImage.get(), data, x * y * c * sizeof(uint8_t), Arc::ImageLayout::General);
		stbi_image_free(data);
	}
	else
	{
		m_Device->TransitionImageLayout(m_SeedImage.get(), Arc::ImageLayout::General);
		float clearColor[4] = {0, 0, 0, 0};
		m_Device->ClearColorImage(m_SeedImage.get(), clearColor, Arc::ImageLayout::General);
	}

	m_JFAImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_JFAImage.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1},
		.Format = Arc::Format::R16G16_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	m_Device->TransitionImageLayout(m_JFAImage.get(), Arc::ImageLayout::General);

	m_SDFImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_SDFImage.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1},
		.Format = Arc::Format::R16_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	m_Device->TransitionImageLayout(m_SDFImage.get(), Arc::ImageLayout::General);

	m_NearestColorImage = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_NearestColorImage.get(), Arc::GpuImageDesc{
		.Extent = { w, h, 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	m_Device->TransitionImageLayout(m_NearestColorImage.get(), Arc::ImageLayout::General);


	// Gets size that is equal or less that (w, h) and is divisible by 2^(cascadeCount - 1)
	// 2^(cascadeCount - 1) is how many probe groups are one one side of the cascade
	uint32_t cascadeCount = 6;
	uint32_t div = 1 << (cascadeCount - 1);
	glm::uvec2 cascadeExtent = { w - (w % div), h - (h % div)};
	m_Cascades.resize(cascadeCount);
	for (auto& cascade : m_Cascades)
	{
		m_ResourceCache->CreateGpuImage(&cascade, Arc::GpuImageDesc{
			.Extent = { cascadeExtent.x, cascadeExtent.y, 1},
			.Format = Arc::Format::R8G8B8A8_Unorm,
			.UsageFlags = Arc::ImageUsage::TransferSrc | Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled,
			.AspectFlags = Arc::ImageAspect::Color,
			});
		m_Device->TransitionImageLayout(&cascade, Arc::ImageLayout::General);
	}
}

RadianceCascades::~RadianceCascades()
{

}

void RadianceCascades::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	lastTime = elapsedTime;

	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	jfaData.position = { Arc::Input::GetMouseX(), Arc::Input::GetMouseY() };
	jfaData.radius = Arc::Input::IsKeyDown(Arc::KeyCode::MouseLeft) ? 15.0f : 0.0f;
	jfaData.clear = Arc::Input::IsKeyPressed(Arc::KeyCode::C) ? 1.0f : 0.0f;
	jfaData.color = { HsvToRgb(elapsedTime * 0.1f, 1, 1), 1 };
	if (Arc::Input::IsKeyDown(Arc::KeyCode::MouseRight))
		jfaData.color = { 1, 1, 1, 1 };
	else if (Arc::Input::IsKeyDown(Arc::KeyCode::A))
		jfaData.color = { 0, 0, 0, 1 };

	if (Arc::Input::IsKeyPressed(Arc::KeyCode::G))
	{
		std::vector<uint8_t> imageData = m_Device->GetImageData(&m_Cascades[0]);
		std::string path = "img.png";
		stbi_write_png(path.c_str(), m_Cascades[0].GetExtent()[0], m_Cascades[0].GetExtent()[1], 4, imageData.data(), m_Cascades[0].GetExtent()[0] * 4);
		ARC_LOG("Screenshot saved to disk");
	}


	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindComputePipeline(m_JFAPipeline->GetHandle());

			for (int i = 0; i < 3; i++)
			{
				jfaData.iteration = (i == 2) ? -1 : i;

				cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_JFAPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
					.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::StorageImage, m_SeedImage->GetImageView(), Arc::ImageLayout::General, nullptr))
					.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_JFAImage->GetImageView(), Arc::ImageLayout::General, nullptr))
					.AddWrite(Arc::PushImageWrite(2, Arc::DescriptorType::StorageImage, m_SDFImage->GetImageView(), Arc::ImageLayout::General, nullptr))
					.AddWrite(Arc::PushImageWrite(3, Arc::DescriptorType::StorageImage, m_NearestColorImage->GetImageView(), Arc::ImageLayout::General, nullptr)));
				cmd->PushConstants(Arc::ShaderStage::Compute, m_JFAPipeline->GetLayout(), &jfaData, sizeof(jfaData));
				cmd->Dispatch(std::ceil(m_SeedImage->GetExtent()[0] / 32.0f), std::ceil(m_SeedImage->GetExtent()[1] / 32.0f), 1);
			}

			cmd->BindComputePipeline(m_RadianceCascadesPipeline->GetHandle());
			for (int i = m_Cascades.size() - 1; i >= 0; i--)
			{
				cascadeData.Index = i;
				cascadeData.Interval = 2.0f;
				cascadeData.Count = m_Cascades.size();
				float cascadeIndex = i;
				cmd->PushDescriptorSets(Arc::PipelineBindPoint::Compute, m_RadianceCascadesPipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
					.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::CombinedImageSampler, m_SDFImage->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle()))
					.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::CombinedImageSampler, m_NearestColorImage->GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle()))
					.AddWrite(Arc::PushImageWrite(2, Arc::DescriptorType::StorageImage, m_Cascades[i].GetImageView(), Arc::ImageLayout::General, nullptr))
					.AddWrite(Arc::PushImageWrite(3, Arc::DescriptorType::CombinedImageSampler, m_Cascades[std::min(i + 1, (int)m_Cascades.size() - 1)].GetImageView(), Arc::ImageLayout::General, m_LinearSampler->GetHandle())));
				cmd->PushConstants(Arc::ShaderStage::Compute, m_RadianceCascadesPipeline->GetLayout(), &cascadeData, sizeof(cascadeData));
				cmd->Dispatch(m_Cascades[0].GetExtent()[0] / 32.0f, m_Cascades[0].GetExtent()[1] / 32.0f, 1);
			}
		}
	});

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {0.1, 0.6, 0.6, 1.0},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindPipeline(m_CompositePipeline->GetHandle());
			cmd->PushDescriptorSets(Arc::PipelineBindPoint::Graphics, m_CompositePipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
				.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::CombinedImageSampler, m_SeedImage->GetImageView(), Arc::ImageLayout::General, m_NearestSampler->GetHandle()))
				.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::CombinedImageSampler, m_Cascades[0].GetImageView(), Arc::ImageLayout::General, m_NearestSampler->GetHandle())));
			cmd->Draw(6, 1, 0, 0);
		}
	});

	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
	m_Device->WaitIdle();
	//m_Device->GetTimestampQuery()->QueryResults();
}

glm::vec3 RadianceCascades::HsvToRgb(float h, float s, float v)
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

void RadianceCascades::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
}

void RadianceCascades::RecompileShaders()
{

}

void RadianceCascades::WaitForFrameEnd()
{

}