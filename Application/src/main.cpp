#include "Window/Window.h"
#include "Window/Input.h"
#include "Graphics/Device.h"
#include "Graphics/PresentQueue.h"
#include "VolumeRendering/VolumeRenderer.h"
#include "Core/Timer.h"
#include "Core/Log.h"
#include <memory>
#include <iostream>

void Run()
{
	Arc::WindowDescription windowDesc;
	windowDesc.Title = "Arcane Vulkan renderer";
	windowDesc.Width = 1280;
	windowDesc.Height = 720;
	windowDesc.Fullscreen = false;
	auto window = std::make_unique<Arc::Window>(windowDesc);

	Arc::SurfaceDesc winDesc = {};
	winDesc.hInstance = window->GetHInstance();
	winDesc.hWnd = window->GetHWnd();

	uint32_t inFlightImageCount = 3;
	Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;

	auto device = std::make_unique<Arc::Device>(window->GetExtensions(), winDesc, inFlightImageCount);
	auto presentQueue = std::make_unique<Arc::PresentQueue>(device.get(), winDesc, presentMode);

	std::unique_ptr<BaseRenderer> renderer;

	renderer = std::make_unique<VolumeRenderer>(window.get(), device.get(), presentQueue.get());

	Arc::Timer timer;
	while (!window->IsClosed())
	{
		window->PollEvents();

		if (Arc::Input::IsKeyPressed(Arc::KeyCode::Escape))
			window->SetClosed(true);
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F1))
			window->SetFullscreen(!window->IsFullscreen());
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F2))
			renderer->RecompileShaders();

		if (presentQueue->GetSwapchain()->OutOfDate())
		{
			while (window->Width() == 0 || window->Height() == 0)
				window->WaitEvents();

			renderer->WaitForFrameEnd();
			presentQueue.reset();
			presentQueue = std::make_unique<Arc::PresentQueue>(device.get(), winDesc, presentMode);
			ARC_LOG("Swapchain recreated!");
			renderer->SwapchainResized(presentQueue.get());
		}

		renderer->RenderFrame(timer.elapsed_sec());
	}
}

int main()
{
	Run();

	return 0;
}

#if FALSE
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	Run();

	return 0;
}
#endif