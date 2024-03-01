#include "VolumeRenderer.h"
#include "Graphics/ImGui/ImguiWrapper.h"
#include <fstream>
#include "stb/stb_image.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Graphics/Vulkan/ShaderCompiler.h"

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


	//std::ifstream file("res/Volume/body_512x512x226.raw", std::ios::ate | std::ios::binary);
	std::ifstream file("res/Volume/manix.raw", std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	file.seekg(0);
	//uint16_t sizeX = 512, sizeY = 512, sizeZ = 226;
	uint16_t sizeX = 512, sizeY = 512, sizeZ = 460;
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

	uint16_t* data = new uint16_t[sizeX * sizeY * sizeZ];
	file.read((char*)data, sizeof(uint16_t) * sizeX * sizeY * sizeZ);
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


	DensityPoint dp;
	dp.density = 0.0f;
	dp.location = 0.05f;
	dp.color = { 1, 0, 0 };
	m_Points.push_back(dp);
	dp.density = 1.0f;
	dp.location = 0.65f;
	dp.color = { 1.0, 0.9, 0.0 };
	m_Points.push_back(dp);
	dp.density = 1.0f;
	dp.location = 1.0f;
	dp.color = { 0.0, 1.0, 1.0 };
	m_Points.push_back(dp);

	uint32_t gradSize = 256;
	m_GradientData.resize(gradSize * 4);

	CalculateDataPoints();

	m_ColorGradientImage = std::make_unique<Arc::Image>();
	core->GetResourceCache()->CreateImage(m_ColorGradientImage.get(), Arc::ImageDesc()
		.SetExtent({ (uint32_t)gradSize, (uint32_t)1 })
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled));
	core->SetImageData(m_ColorGradientImage.get(), m_GradientData.data(), gradSize * 1 * 4, Arc::ImageLayout::ShaderReadOnlyOptimal);

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
		.AddBinding(1, Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute)
		.AddBinding(1, Arc::DescriptorType::StorageImage, Arc::ShaderStage::Compute));
	m_Device->UpdateDescriptorSet(m_ComputeDescriptor.get(), Arc::DescriptorWriteDesc()
		.AddImageWrite(0, m_LinearSampler->GetHandle(), texture.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(1, m_LinearSampler->GetHandle(), m_ColorGradientImage->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(2, m_PointSampler->GetHandle(), hdrTexture.GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal)
		.AddImageWrite(3, m_PointSampler->GetHandle(), m_AccumulationImage->GetImageView(), Arc::ImageLayout::General)
		.AddImageWrite(4, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_ComputeAttachment.get(), Arc::ImageLayout::General);

	BuildRenderGraph();

	PreparePipelines();

	m_Camera = std::make_unique<CameraFPS>(window);
	m_Camera->Position = glm::vec3(0.5, 0.5, -0.5f);
	m_Camera->MovementSpeed = 0.5f;

	m_RenderThreadFuture = std::promise<void>().get_future();
}

void VolumeRenderer::CalculateDataPoints()
{
	std::vector<DensityPoint> pointsCopy;
	DensityPoint dp;
	dp.density = 0.0f;
	dp.location = 0.0f;
	dp.color = { 0, 0, 0 };
	pointsCopy.push_back(dp);

	for (size_t i = 0; i < m_Points.size(); i++)
	{
		pointsCopy.push_back(m_Points[i]);
	}

	dp.density = 0.0f;
	dp.location = 1.0f;
	dp.color = { 0, 0, 0 };
	pointsCopy.push_back(dp);


	std::sort(pointsCopy.begin(), pointsCopy.end(), [](DensityPoint a, DensityPoint b) { return a.location < b.location; });

	for (size_t i = 0; i < 256; i++)
	{
		float f = i / float(256);
		DensityPoint left = pointsCopy[0];
		DensityPoint right = pointsCopy[pointsCopy.size() - 1];

		for (size_t j = 0; j < pointsCopy.size() - 1; j++)
		{
			if (pointsCopy[j].location >= f)
				break;
			left = pointsCopy[j];
			right = pointsCopy[j + 1];
		}

		float lerpDensity = (left.density * (right.location - f) + right.density * (f - left.location)) / (right.location - left.location);
		glm::vec3 lerpColor = (left.color * (right.location - f) + right.color * (f - left.location)) / (right.location - left.location);
		m_GradientData[i * 4 + 0] = lerpColor.r * 255.0f;
		m_GradientData[i * 4 + 1] = lerpColor.g * 255.0f;
		m_GradientData[i * 4 + 2] = lerpColor.b * 255.0f;
		m_GradientData[i * 4 + 3] = lerpDensity * 255.0f;
	}
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

	bool changed = false;
	if (ImGui::BeginListBox("Data points"))
	{
		ImGui::PushItemWidth(84);
		for (size_t i = 0; i < m_Points.size(); i++)
		{	
			std::string s1 = "##L" + std::to_string(i);
			std::string s2 = "##D" + std::to_string(i);
			std::string s3 = "##C" + std::to_string(i);

			changed |= ImGui::SliderFloat(s1.c_str(), &m_Points[i].location, 0.0f, 1.0f);
			ImGui::SameLine();
			changed |= ImGui::SliderFloat(s2.c_str(), &m_Points[i].density, 0.0f, 1.0f);
			ImGui::SameLine();
			changed |= ImGui::ColorEdit3(s3.c_str(), &m_Points[i].color.r, ImGuiColorEditFlags_NoInputs);
		}
		ImGui::PopItemWidth();
		ImGui::EndListBox();
	}

	if (ImGui::Button("Add"))
	{
		DensityPoint dp;
		dp.density = 0.0f;
		dp.location = 0.0f;
		dp.color = { 1.0, 1.0, 1.0 };
		m_Points.push_back(dp);

		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove"))
	{
		if (m_Points.size() >= 1)
			m_Points.pop_back();
		changed = true;
	}
	ImGui::SameLine();
	ImGui::Text("Density point");

	if (changed)
	{
		ARC_LOG("Changed desity points!");
		CalculateDataPoints();
		m_Device->SetImageData(m_ColorGradientImage.get(), m_GradientData.data(), m_GradientData.size(), Arc::ImageLayout::ShaderReadOnlyOptimal);
	}

	bool val1 = ImGui::SliderInt("Bounce limit", &m_CameraFrameData.bounceLimit, 1, 128);
	bool val2 = ImGui::SliderFloat("Extinction", &m_CameraFrameData.extinction, 0.0f, 100.0f);
	bool val3 = ImGui::SliderFloat("Anisotropy", &m_CameraFrameData.anisotropy, -1.0f, 1.0f);
	bool val4 = ImGui::ColorEdit3("Background", &m_CameraFrameData.backgroundColor.r);
	ImGui::End();
	
	if (m_Camera->HasMoved ||
		changed ||
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

void VolumeRenderer::CompileShaders()
{
	m_VolumeComputeShader = std::make_unique<Arc::Shader>();
	m_PresentVertexShader = std::make_unique<Arc::Shader>();
	m_PresentFragmentShader = std::make_unique<Arc::Shader>();

	Arc::ShaderCompiler::Initialize();
	Arc::ShaderDesc shaderDesc;
	Arc::ShaderCompiler::Compile("res/Shaders/VolumetricRendering/Volume.comp", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_VolumeComputeShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/VolumetricRendering/Present.vert", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_PresentVertexShader.get(), shaderDesc);
	Arc::ShaderCompiler::Compile("res/Shaders/VolumetricRendering/Present.frag", shaderDesc);
	m_Device->GetResourceCache()->CreateShader(m_PresentFragmentShader.get(), shaderDesc);
	Arc::ShaderCompiler::Finalize();

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

	Arc::ShaderCompiler::Initialize();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/VolumetricRendering/Volume.comp", shaderDesc))
	{
		m_Device->GetResourceCache()->ReleaseResource(m_VolumeComputeShader.get());
		m_Device->GetResourceCache()->CreateShader(m_VolumeComputeShader.get(), shaderDesc);	

		m_Device->GetResourceCache()->ReleaseResource(m_ComputePipeline.get());
		m_Device->GetResourceCache()->CreateComputePipeline(m_ComputePipeline.get(), Arc::ComputePipelineDesc()
			.SetShaderStage(m_VolumeComputeShader.get()));
	}

	Arc::ShaderCompiler::Finalize();

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
		.AddImageWrite(3, m_PointSampler->GetHandle(), m_AccumulationImage->GetImageView(), Arc::ImageLayout::General)
		.AddImageWrite(4, m_PointSampler->GetHandle(), m_ComputeAttachment->GetImageView(), Arc::ImageLayout::General));

	m_Device->TransitionImageLayout(m_AccumulationImage.get(), Arc::ImageLayout::General);
	m_Device->TransitionImageLayout(m_ComputeAttachment.get(), Arc::ImageLayout::General);

	BuildRenderGraph();

	m_CameraFrameData.frameIndex = 0;
}