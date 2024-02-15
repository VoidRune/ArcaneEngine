#include "VolumeRenderer.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include <fstream>
#include "stb/stb_image.h"
#include "Graphics/Vulkan/SpirvCompiler.h"
#include "Core/Log.h"
#include "Core/Timer.h"

VolumeRenderer::VolumeRenderer(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = core;
	m_PresentQueue = presentQueue;
	Arc::ImGuiInit(window->GetHandle(), core, presentQueue);

	m_WindowSize = presentQueue->GetSwapchain()->GetExtent();

	CompileShaders();

	/* Prepare global data bound at the start of a frame */
	/* Camera */
	m_CameraFrameDataBuffer = std::make_unique<Arc::InFlightBuffer>();
	m_Device->GetResourceCache()->CreateInFlightBuffer(m_CameraFrameDataBuffer.get(), Arc::BufferDesc()
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


	std::ifstream file("res/Volume/body_512x512x226.raw", std::ios::ate | std::ios::binary);
	//std::ifstream file("res/Volume/manix.raw", std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	file.seekg(0);
	uint16_t sizeX = 512, sizeY = 512, sizeZ = 226;
	//uint16_t sizeX = 512, sizeY = 512, sizeZ = 460;
	//file.read((char*)&sizeX, sizeof(sizeX));
	//file.read((char*)&sizeY, sizeof(sizeY));
	//file.read((char*)&sizeZ, sizeof(sizeZ));

	bool flipYZ = true;
	bool invertY = false;
	if (flipYZ)
	{
		uint32_t temp = sizeY;
		sizeY = sizeZ;
		sizeZ = temp;
	}

	uint8_t* data = new uint8_t[sizeX * sizeY * sizeZ];
	file.read((char*)data, sizeof(uint8_t) * sizeX * sizeY * sizeZ);
	file.close();

	std::vector<uint8_t> imageData1(sizeX * sizeY * sizeZ);

	for (size_t z = 0; z < sizeZ; z++)
	{
		for (size_t y = 0; y < sizeY; y++)
		{
			for (size_t x = 0; x < sizeX; x++)
			{
				uint32_t indexA = x + z * sizeX + y * sizeX * sizeZ;
				uint32_t indexB = x + z * sizeX + y * sizeX * sizeZ;
				if (flipYZ)
					indexA = x + y * sizeX + z * sizeX * sizeY;
				if (invertY)
					indexB = x + z * sizeX + (sizeY - y - 1) * sizeX * sizeZ;
				imageData1[indexA] = data[indexB];
			}
		}
	}

	//int width, height, nrChannels;
	//std::vector<uint8_t> imageData1;
	//LoadImageData("res/Textures/stoneWall.png", imageData1, &width, &height, &nrChannels);
	Arc::Image texture;
	core->GetResourceCache()->CreateImage(&texture, Arc::ImageDesc()
		.SetExtent({ (uint32_t)sizeX, (uint32_t)sizeY })
		.SetDepth(sizeZ)
		.SetFormat(Arc::Format::R8_Unorm)
		.SetEnableMipLevels(false)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	core->SetImageData(&texture, imageData1.data(), sizeX * sizeY * sizeZ, Arc::ImageLayout::ShaderReadOnlyOptimal);
	delete data;

	m_Device->UpdateDescriptorSet(m_BindlessTexturesDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_LinearSampler->GetHandle(), texture.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal, 1));


	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* hdrData = stbi_loadf("res/Textures/puresky.hdr", &width, &height, &nrComponents, 4);
	Arc::Image hdrTexture;
	core->GetResourceCache()->CreateImage(&hdrTexture, Arc::ImageDesc()
		.SetExtent({ (uint32_t)width, (uint32_t)height })
		.SetFormat(Arc::Format::R32G32B32A32_Sfloat)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	core->SetImageData(&hdrTexture, hdrData, width * height * 4 * 4, Arc::ImageLayout::ShaderReadOnlyOptimal);
	stbi_image_free(hdrData);



	uint32_t gradSize = 256;
	std::vector<uint8_t> gradientData(gradSize * 4);
	for (size_t i = 0; i < gradSize; i++)
	{
		gradientData[i * 4 + 0] = 255 - i;
		gradientData[i * 4 + 1] = 0;
		gradientData[i * 4 + 2] = i;
		gradientData[i * 4 + 3] = 255;
	}

	m_ColorGradientImage = std::make_unique<Arc::Image>();
	core->GetResourceCache()->CreateImage(m_ColorGradientImage.get(), Arc::ImageDesc()
		.SetExtent({ (uint32_t)gradSize, (uint32_t)1 })
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	core->SetImageData(m_ColorGradientImage.get(), gradientData.data(), gradSize * 1 * 4, Arc::ImageLayout::ShaderReadOnlyOptimal);

	std::vector<DensityPoint> points;
	DensityPoint dp;
	dp.density = 0.0f;
	dp.location = 0.0f;
	dp.color = { 1, 1, 1 };
	points.push_back(dp);
	dp.density = 1.0f;
	dp.location = 0.5f;
	points.push_back(dp);
	dp.density = 0.5f;
	dp.location = 1.0f;
	points.push_back(dp);
	uint32_t pointIndex = 0;

	m_DensityRemap.resize(gradSize);
	//for (size_t i = 0; i < gradSize; i++)
	//{
	//	float location = (float)i / gradSize;
	//
	//	if()
	//	pointIndex
	//	std::cout << location << std::endl;
	//	m_densityRemap[i] = 255;
	//}

	auto current = points.begin();
	auto next = std::next(current);
	float step = 1.0f / (gradSize - 1);

	for (int i = 0; i < gradSize; ++i) {
		// Find the two points between which the current 'location' lies
		/*
		while (next != points.end() && (*next).location <= i * step) {
			current = next;
			++next;
		}

		// Calculate interpolation factor
		float t = 0.0f;
		if (next != points.end()) {
			t = (i * step - (*current).location) / ((*next).location - (*current).location);
		}

		// Perform linear interpolation for density and store in m_densityRemap
		float interpolatedDensity = ((*next).density - (*current).density) * t + (*current).density;
		m_densityRemap[i] = static_cast<uint8_t>(interpolatedDensity * 255.0f);
		std::cout << interpolatedDensity << std::endl;
		*/
		m_DensityRemap[i] = 255.0f;
	}

	m_DensityImage = std::make_unique<Arc::Image>();
	core->GetResourceCache()->CreateImage(m_DensityImage.get(), Arc::ImageDesc()
		.SetExtent({ (uint32_t)gradSize, (uint32_t)1 })
		.SetFormat(Arc::Format::R8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	core->SetImageData(m_DensityImage.get(), m_DensityRemap.data(), gradSize * sizeof(uint8_t), Arc::ImageLayout::ShaderReadOnlyOptimal);


	m_ImGuiDset = ImGui_ImplVulkan_AddTexture(m_LinearSampler->GetHandle(), m_ColorGradientImage->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


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
		.AddImageWrite(0, m_LinearSampler->GetHandle(), texture.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(1, m_LinearSampler->GetHandle(), m_ColorGradientImage->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(2, m_LinearSampler->GetHandle(), m_DensityImage->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(3, m_PointSampler->GetHandle(), hdrTexture.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
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

VolumeRenderer::~VolumeRenderer()
{
	m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
	Arc::ImGuiShutdown();
	m_Device->GetResourceCache()->PrintHeapBudgets();
	m_Device->GetResourceCache()->FreeResources();
}

void VolumeRenderer::RenderFrame(float elapsedTime)
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


	float points[256];
	for (size_t i = 0; i < 256; i++)
	{
		points[i] = m_DensityRemap[i] / 255.0f;
	}

	ImGui::PlotLines("Density", points, m_DensityRemap.size());

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

void VolumeRenderer::CompileShaders()
{
	m_VolumeComputeShader = std::make_unique<Arc::Shader>();
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();
	SpirvHelper::Init();
	m_Device->GetResourceCache()->CreateShader(m_VolumeComputeShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/VolumetricRendering/Volume.comp"));
	m_Device->GetResourceCache()->CreateShader(m_PresentVertexShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/VolumetricRendering/Present.vert"));
	m_Device->GetResourceCache()->CreateShader(m_PresentFragmentShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/VolumetricRendering/Present.frag"));
	SpirvHelper::Finalize();
}

void VolumeRenderer::PreparePipelines()
{
	Arc::VertexAttributes emptyAttributes({ }, 0);

	m_ComputePipeline = std::make_unique<Arc::ComputePipeline>();
	m_Device->GetResourceCache()->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc()
		.SetShaderStage(m_VolumeComputeShader.get()));

	m_PresentPipeline = std::make_unique<Arc::Pipeline>();
	m_Device->GetResourceCache()->CreatePipeline(m_PresentPipeline.get(), Arc::PipelineDesc()
		.AddShaderStage(m_PresentVertexShader.get())
		.AddShaderStage(m_PresentFragmentShader.get())
		.SetPrimitiveTopology(Arc::PrimitiveTopology::TriangleFan)
		.SetRenderPass(m_Device->GetRenderGraph()->GetRenderPassHandle(m_PresentPassProxy))
		.SetVertexAttributes(emptyAttributes));

}

void VolumeRenderer::BuildRenderGraph()
{
	m_ComputePassProxy = m_Device->GetRenderGraph()->AddPass("compute", Arc::ComputePassDesc()
		.SetOutputImages({ m_AccumulationImage->GetProxy() }));

	m_PresentPassProxy = m_Device->GetRenderGraph()->AddPass("present", Arc::PresentPassDesc()
		.SetFormat(m_PresentQueue->GetSwapchain()->GetSurfaceFormat().format)
		.SetImageViews(m_PresentQueue->GetSwapchain()->GetImageViews())
		.SetExtent(m_PresentQueue->GetSwapchain()->GetExtent()));

	m_Device->GetRenderGraph()->BuildGraph();

}

void VolumeRenderer::WaitForFrameEnd()
{
	m_RenderThreadFuture.wait();
	m_Device->WaitIdle();
}

void VolumeRenderer::RecompileShaders()
{
	WaitForFrameEnd();

	Arc::Timer timer;

	m_Device->GetResourceCache()->ReleaseResource(m_VolumeComputeShader.get());
	SpirvHelper::Init();
	m_Device->GetResourceCache()->CreateShader(m_VolumeComputeShader.get(), Arc::ShaderDesc().SetFilePath("res/Shaders/VolumetricRendering/Volume.comp"));
	SpirvHelper::Finalize();

	m_Device->GetResourceCache()->ReleaseResource(m_ComputePipeline.get());

	m_Device->GetResourceCache()->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc()
		.SetShaderStage(m_VolumeComputeShader.get()));

	ARC_LOG("Recompiled shaders! " + std::to_string(timer.elapsed_mili()) + "ms");

	m_CameraFrameData.frameIndex = 0;
}

void VolumeRenderer::SwapchainResized(void* presentQueue)
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