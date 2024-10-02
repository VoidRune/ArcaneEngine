#define ARCANE_ENABLE_VALIDATION

#include <memory>
#include <vector>
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include "ArcaneEngine/Core/Timer.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"

int main()
{
	Arc::WindowDescription windowDesc;
	windowDesc.Title = "Arcane Vulkan renderer";
	windowDesc.Width = 1280;
	windowDesc.Height = 720;
	windowDesc.Fullscreen = false;
	auto window = std::make_unique<Arc::Window>(windowDesc);

	uint32_t inFlightFrameCount = 3;
	Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;

	auto device = std::make_unique<Arc::Device>(window->GetHandle(), window->GetInstanceExtensions(), inFlightFrameCount);
	auto presentQueue = std::make_unique<Arc::PresentQueue>(device.get(), presentMode);
	auto resourceCache = std::make_unique<Arc::ResourceCache>(device.get());
	auto renderGraph = std::make_unique<Arc::RenderGraph>();

	std::unique_ptr<Arc::GpuImage> image = std::make_unique<Arc::GpuImage>();
	resourceCache->CreateGpuImage(image.get(), Arc::GpuImageDesc{
		.Extent = {windowDesc.Width, windowDesc.Height, 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::ColorAttachment | Arc::ImageUsage::TransferSrc | Arc::ImageUsage::TransferDst,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
		});

	struct GlobalFrameData
	{
		float r, g, b;
	} globalFrameData;

	std::unique_ptr<Arc::GpuBufferArray> bufferArray = std::make_unique<Arc::GpuBufferArray>();
	resourceCache->CreateGpuBufferArray(bufferArray.get(), Arc::GpuBufferDesc{
		.Size = sizeof(GlobalFrameData),
		.UsageFlags = Arc::BufferUsage::UniformBuffer,
		.MemoryProperty = Arc::MemoryProperty::HostVisible,
		});

	std::unique_ptr<Arc::DescriptorSetArray> descSetArray = std::make_unique<Arc::DescriptorSetArray>();
	resourceCache->AllocateDescriptorSetArray(descSetArray.get(), Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::UniformBuffer, Arc::ShaderStage::Vertex }
		}
		});

	device->UpdateDescriptorSet(descSetArray.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::BufferArrayWrite(0, bufferArray.get()))
	);

	Arc::Shader vertShader;
	Arc::Shader fragShader;
	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/shader.vert", shaderDesc))
		resourceCache->CreateShader(&vertShader, shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/shader.frag", shaderDesc))
		resourceCache->CreateShader(&fragShader, shaderDesc);

	Arc::Pipeline pipeline;
	resourceCache->CreatePipeline(&pipeline, Arc::PipelineDesc{
		.ShaderStages = { &vertShader, &fragShader }
	});

	Arc::Timer timer;
	while (!window->IsClosed())
	{
		window->PollEvents();
		Arc::FrameData frameData = presentQueue->BeginFrame();
		Arc::CommandBuffer* cmd = frameData.CommandBuffer;

		float timeValue = (float)timer.elapsed_sec();
		globalFrameData = { 0.2f, timeValue - (int)timeValue, 1.0f};
		{
			void* data = resourceCache->MapMemory(bufferArray.get(), frameData.FrameIndex);
			memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
			resourceCache->UnmapMemory(bufferArray.get(), frameData.FrameIndex);
		}

		renderGraph->SetPresentPass(Arc::PresentPass{
			.LoadOp = Arc::AttachmentLoadOp::Clear,
			.ClearColor = {1, 0.5, 0, 1},
			.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
				cmd->BindDescriptorSets(Arc::PipelineBindPoint::Graphics, pipeline.GetLayout(), 0, { descSetArray->GetHandle(frameData.FrameIndex)});
				cmd->BindPipeline(pipeline.GetHandle());
				cmd->Draw(3, 1, 0, 0);
			}
		});
		renderGraph->BuildGraph();
		renderGraph->Execute(frameData, presentQueue->GetExtent());
		presentQueue->EndFrame();
	}
	device->WaitIdle();
	resourceCache->FreeResources();
}