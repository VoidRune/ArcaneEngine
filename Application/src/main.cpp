#define ARCANE_ENABLE_VALIDATION

#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Core/Timer.h"

#include "VolumerRenderer/VolumeRenderer.h"
#include "RadianceCascades/RadianceCascades.h"

int currentRendererId = -1;
void GetRenderer(int rendererId, std::unique_ptr<RendererBase>& renderer, 
				 Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	if (currentRendererId == rendererId)
		return;

	switch (rendererId)
	{
		case 1: 
			device->WaitIdle();
			device->GetResourceCache()->FreeResources();
			renderer.reset();
			renderer = std::make_unique<VolumeRenderer>(window, device, presentQueue);
			break;
		case 2: 
			device->WaitIdle();
			device->GetResourceCache()->FreeResources();
			renderer.reset();
			renderer = std::make_unique<RadianceCascades>(window, device, presentQueue);
			break;
	}
	currentRendererId = rendererId;
}

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

	std::unique_ptr<RendererBase> renderer;
	GetRenderer(2, renderer, window.get(), device.get(), presentQueue.get());
	
	Arc::Timer timer;
	while (!window->IsClosed())
	{
		window->PollEvents();

		if (Arc::Input::IsKeyPressed(Arc::KeyCode::Key1)) GetRenderer(1, renderer, window.get(), device.get(), presentQueue.get());
		else if (Arc::Input::IsKeyPressed(Arc::KeyCode::Key2)) GetRenderer(2, renderer, window.get(), device.get(), presentQueue.get());

		if (Arc::Input::IsKeyPressed(Arc::KeyCode::Escape))
			window->SetClosed(true);
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F1))
			window->SetFullscreen(!window->IsFullscreen());
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F2))
			renderer->RecompileShaders();

		if (presentQueue->OutOfDate())
		{
			while (window->Width() == 0 || window->Height() == 0)
				window->WaitEvents();
			device->WaitIdle();
			presentQueue.reset();
			presentQueue = std::make_unique<Arc::PresentQueue>(device.get(), presentMode);
			renderer->SwapchainResized(presentQueue.get());
		}

		renderer->RenderFrame(timer.elapsed_sec());
	}
	device->WaitIdle();
	device->GetResourceCache()->FreeResources();
}