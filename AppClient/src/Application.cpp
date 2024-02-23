#include "Application.h"
#include "Core/Log.h"
#include "Core/Timer.h"

#include "Core/BufferStream.h"
#include "PacketType.h"
#include <iostream>

Application::Application()
{
	Arc::WindowDescription windowDesc;
	windowDesc.Title = "Arcane Vulkan renderer";
	windowDesc.Width = 1280;
	windowDesc.Height = 720;
	windowDesc.Fullscreen = false;
	m_Window = std::make_unique<Arc::Window>(windowDesc);

	Arc::SurfaceDesc winDesc = {};
	winDesc.hInstance = m_Window->GetHInstance();
	winDesc.hWnd = m_Window->GetHWnd();

	uint32_t inFlightImageCount = 3;
	Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;

	m_Device = std::make_unique<Arc::Device>(m_Window->GetExtensions(), winDesc, inFlightImageCount);
	m_PresentQueue = std::make_unique<Arc::PresentQueue>(m_Device.get(), winDesc, presentMode);
	m_Renderer = std::make_unique<Renderer>(m_Window.get(), m_Device.get(), m_PresentQueue.get());
	m_AssetCache = std::make_unique<AssetCache>(m_Device.get());

	std::string ip = "127.0.0.1:60000";

	m_Client = std::make_unique<Arc::Client>();
	m_Client->SetServerConnectedCallback([this]() { OnConnected(); });
	m_Client->SetServerDisconnectedCallback([this]() { OnDisconnected(); });
	m_Client->SetDataReceivedCallback([this](const Arc::Buffer data) { OnDataReceived(data); });
	m_Client->ConnectToServer(ip.c_str());

	m_Camera = std::make_unique<CameraFPS>(m_Window.get());
	m_Camera->Position = glm::vec3(0.0, 0.0, -2.0f);
	m_Camera->MovementSpeed = 1.0f;

	m_AssetCache->LoadMesh(&m_Mesh, "res/Meshes/cylinder.obj");

	m_ScratchBuffer.Allocate(4096);
}

Application::~Application()
{
	m_Client->Disconnect();
}

void Application::Run()
{
	Arc::Timer timer;
	while (!m_Window->IsClosed())
	{
		m_Window->PollEvents();

		if (Arc::Input::IsKeyPressed(Arc::KeyCode::Escape))
			m_Window->SetClosed(true);
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F1))
			m_Window->SetFullscreen(!m_Window->IsFullscreen());
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::F2))
			m_Renderer->RecompileShaders();

		if (m_PresentQueue->GetSwapchain()->OutOfDate())
		{
			while (m_Window->Width() == 0 || m_Window->Height() == 0)
				m_Window->WaitEvents();

			m_Renderer->WaitForFrameEnd();
			m_PresentQueue.reset();

			Arc::SurfaceDesc winDesc = {};
			winDesc.hInstance = m_Window->GetHInstance();
			winDesc.hWnd = m_Window->GetHWnd();
			Arc::PresentMode presentMode = Arc::PresentMode::Mailbox;

			m_PresentQueue = std::make_unique<Arc::PresentQueue>(m_Device.get(), winDesc, presentMode);
			ARC_LOG("Swapchain recreated!");
			m_Renderer->SwapchainResized(m_PresentQueue.get());
		}

		float elapsedTime = timer.elapsed_sec();
		float dt = elapsedTime - m_LastTime;
		m_LastTime = elapsedTime;
		m_Camera->Update(dt);
		Renderer::FrameData frameData;
		frameData.View = m_Camera->View;
		frameData.Projection = m_Camera->Projection;

		for (auto& player : m_Players)
		{
			//frameData.Models.push_back(Model(m_Mesh, player.second.Position));
			std::cout << player.second.Position.x << " " << player.second.Position.y << " " << player.second.Position.z << std::endl;
		}
		frameData.Models.push_back(Model(m_Mesh, glm::vec3(0, 0, 0)));
		m_Renderer->RenderFrame(frameData);
	}
	m_Renderer->WaitForFrameEnd();
}

void Application::OnConnected()
{
	Arc::LogInfo("Server connected!");

	Arc::BufferStreamWriter stream(m_ScratchBuffer);
	stream.WriteRaw<PacketType>(PacketType::RequestPlayerInfo);
	stream.WriteRaw<uint32_t>(m_Client->GetClientID());
	m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
}

void Application::OnDisconnected()
{
	Arc::LogInfo("Server disconnected!");
}

void Application::OnDataReceived(const Arc::Buffer buffer)
{
	Arc::BufferStreamReader stream(buffer);
	PacketType type;
	stream.ReadRaw<PacketType>(type);
	uint32_t senderId;
	stream.ReadRaw<uint32_t>(senderId);

	Arc::LogInfo(std::to_string(senderId) + " " + std::to_string(buffer.Size));

	switch (type)
	{
	case PacketType::AddPlayer:
	{
		PlayerInfo playerInfo;
		stream.ReadRaw<PlayerInfo>(playerInfo);
		m_Players[senderId] = playerInfo;

		Arc::LogInfo("Add player");
		break;
	}
	case PacketType::RemovePlayer:
	{
		break;
	}
	case PacketType::MovePlayer:
	{
		PlayerInfo playerInfo;
		stream.ReadRaw<PlayerInfo>(playerInfo);
		m_Players[senderId] = playerInfo;

		Arc::LogInfo("Move player");
		break;
	}
	case PacketType::RequestPlayerInfo:
	{
		Arc::BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw<PacketType>(PacketType::AddPlayer);
		stream.WriteRaw<uint32_t>(m_Client->GetClientID());
		stream.WriteRaw<PlayerInfo>(PlayerInfo(m_Camera->Position));
		m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()));

		Arc::LogInfo("Request player");
		break;
	}
	}
}