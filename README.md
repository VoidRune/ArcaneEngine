<div align="center">
  <img src="images/Logo.png" alt="Logo" height="120">
</div>

# ArcaneEngine

Custom C++ renderer using Vulkan API.

## Minimal code example
Here is a minimal code example that creates a window, sets up a rendering context, renders a triangle to the screen, and then cleans up the resources.

```cpp
auto window = std::make_unique<Arc::Window>(Arc::WindowDescription());

uint32_t inFlightFrameCount = 3;
Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;

auto device = std::make_unique<Arc::Device>(window->GetHandle(), window->GetInstanceExtensions(), inFlightFrameCount);
auto presentQueue = std::make_unique<Arc::PresentQueue>(device.get(), presentMode);
auto resourceCache = std::make_unique<Arc::ResourceCache>(device.get());
auto renderGraph = std::make_unique<Arc::RenderGraph>();

std::string vertSource = R"(
	#version 450
	layout(location = 0) out vec3 outColor;

	vec2 positions[3] = vec2[](
		vec2(0.0, -0.5),
		vec2(0.5, 0.5),
		vec2(-0.5, 0.5)
	);
	vec3 colors[3] = vec3[](
		vec3(1.0, 0.0, 0.0),
		vec3(0.0, 1.0, 0.0),
		vec3(0.0, 0.0, 1.0)
	);

	void main()
	{
		gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
		outColor = colors[gl_VertexIndex];
	}
	)";
std::string fragSource = R"(
	#version 450
	layout (location = 0) in vec3 inColor;
	layout (location = 0) out vec4 outFragColor;

	void main() 
	{
		outFragColor = vec4(inColor,1.0f);
	}	
	)";

Arc::Shader vertShader;
Arc::Shader fragShader;
Arc::ShaderDesc shaderDesc;
if (Arc::ShaderCompiler::CompileFromSource(vertSource, Arc::ShaderStage::Vertex, shaderDesc, "vert"))
	resourceCache->CreateShader(&vertShader, shaderDesc);
if (Arc::ShaderCompiler::CompileFromSource(fragSource, Arc::ShaderStage::Fragment, shaderDesc, "frag"))
	resourceCache->CreateShader(&fragShader, shaderDesc);

Arc::Pipeline pipeline;
resourceCache->CreatePipeline(&pipeline, Arc::PipelineDesc{
	.ShaderStages = { &vertShader, &fragShader }
});

while (!window->IsClosed())
{
	window->PollEvents();

	Arc::FrameData frameData = presentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	renderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {1, 0.5, 0, 1},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
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
```

## Build
1. Clone this repository
2. Open the `Scripts/` directory and run the appropriate `Setup` script to generate project files.

Note that Vulkan SDK is not included in this project and needs to be downloaded manually!

## Volume renderer
The application implements a physically-based, unbiased volumetric path tracer using Monte Carlo methods for path sampling. To handle heterogeneous media, null-collision methods introduce a fictitious medium, effectively creating a locally homogeneous representation within the heterogeneous medium.

Delta tracking is employed to sample the distance to the next interaction event, determining whether a particle is absorbed, scattered, or undergoes a null collision based on rejection sampling. For null-collision methods to perform efficiently, they require knowledge of the maximum density, known as the majorant.

To address this, progressive null tracking is implemented, which dynamically stores bounding majorants that gradually converge to their true values over multiple frames. This approach allows for efficient skipping of large sections of the medium with lower density, improving computational performance without sacrificing accuracy.

![Absorption emission](/images/img1.png)
![Multiple scattering](/images/img2.png)

## Dependencies
* [Vulkan SDK](https://vulkan.lunarg.com)
* [GLFW](https://www.glfw.org)
* [GLM](https://glm.g-truc.net/0.9.8/index.html)
* [stb](https://github.com/nothings/stb)

<!--
* [ImGui](https://github.com/ocornut/imgui)
* [miniaudio](https://github.com/mackron/miniaudio)
* [minivorbis](https://github.com/edubart/minivorbis)
* [minimp3](https://github.com/lieff/minimp3)
* [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets)
-->