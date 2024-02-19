#include "Application.h"
#include "Core/Log.h"

#include <iostream>

Application::Application()
{
	m_Client = std::make_unique<Arc::Client>();
	m_Client->SetServerConnectedCallback([this]() { OnConnected(); });
	m_Client->SetServerDisconnectedCallback([this]() { OnDisconnected(); });
	m_Client->SetDataReceivedCallback([this](const Arc::Buffer data) { OnDataReceived(data); });
	m_Client->ConnectToServer("127.0.0.1:60000");
}

Application::~Application()
{
	m_Client->Disconnect();
}

void Application::Update()
{
	std::string msg;
	std::getline(std::cin, msg);

	m_Client->SendBuffer(Arc::Buffer(msg.data(), msg.size()));
}

bool Application::IsRunning()
{
	return true;
}

void Application::OnConnected()
{
	Arc::LogInfo("Server connected!");
}

void Application::OnDisconnected()
{
	Arc::LogInfo("Server disconnected!");
}

void Application::OnDataReceived(const Arc::Buffer buffer)
{
	std::string msg;
	msg.resize(buffer.Size);
	memcpy(msg.data(), buffer.Data, buffer.Size);
	std::cout << msg << std::endl;
}