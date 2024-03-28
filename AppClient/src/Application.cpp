#include "Application.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Renderer/Renderer.h"
#include "Audio/AudioEngine.h"

#include <iostream>
#include <chrono>
#include <thread>

Application::Application()
{
	Arc::WindowDescription windowDesc;
	windowDesc.Title = "Arcane Vulkan renderer";
	windowDesc.Width = 1280;
	windowDesc.Height = 720;
	windowDesc.Fullscreen = false;
	m_Window = std::make_unique<Arc::Window>(windowDesc);

	uint32_t inFlightImageCount = 3;
	Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;

	m_Device = std::make_unique<Arc::Device>(m_Window->GetHandle(), inFlightImageCount);
	m_PresentQueue = std::make_unique<Arc::PresentQueue>(m_Device.get(), presentMode);
	Renderer::Initialize(m_Window.get(), m_Device.get(), m_PresentQueue.get());
	m_AssetCache = std::make_unique<AssetCache>(m_Device.get());
	m_Game = std::make_unique<Game>(m_Window.get(), m_AssetCache.get());


	Arc::AudioEngine::Initialize();
}

Application::~Application()
{
	Arc::AudioEngine::Shutdown();
	Renderer::Shutdown();
}

void Application::Run()
{
	Arc::Timer timer;
	float lastTime = timer.elapsed_sec();
	while (!m_Window->IsClosed())
	{
		m_Window->PollEvents();

		if (Arc::Input::IsKeyPressed(Arc::KeyCode::Escape))
			m_Window->SetClosed(true);
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F1))
			m_Window->SetFullscreen(!m_Window->IsFullscreen());
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F2))
			Renderer::RecompileShaders();

		if (m_PresentQueue->GetSwapchain()->OutOfDate())
		{
			while (m_Window->Width() == 0 || m_Window->Height() == 0)
				m_Window->WaitEvents();

			Renderer::WaitForFrameEnd();
			m_PresentQueue.reset();

			Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;
			m_PresentQueue = std::make_unique<Arc::PresentQueue>(m_Device.get(), presentMode);
			ARC_LOG("Swapchain recreated!");
			Renderer::SwapchainResized(m_PresentQueue.get());
		}

		float elapsedTime = timer.elapsed_sec();
		float deltaTime = elapsedTime - lastTime;
		lastTime = elapsedTime;

		m_Game->Update(deltaTime, elapsedTime, Renderer::GetFrameRenderData());
		Renderer::RenderFrame();
		// TODO remove
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	Renderer::WaitForFrameEnd();
}