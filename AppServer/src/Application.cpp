#include "Application.h"
#include "Core/Log.h"

Application::Application()
{
	m_Server = std::make_unique<Arc::Server>();
	m_Server->SetClientConnectedCallback([this](const Arc::ClientID& clientId) { OnClientConnected(clientId); });
	m_Server->SetClientDisconnectedCallback([this](const Arc::ClientID& clientId) { OnClientDisconnected(clientId); });
	m_Server->SetDataReceivedCallback([this](const Arc::ClientID& clientId, const Arc::Buffer data) { OnDataReceived(clientId, data); });
	m_Server->Start(60000);
}

Application::~Application()
{
	m_Server->Stop();
}

void Application::Update()
{

}

bool Application::IsRunning()
{
	return true;
}

void Application::OnClientConnected(const Arc::ClientID& clientId)
{
	Arc::LogInfo("Client connected! " + std::to_string(clientId));
}

void Application::OnClientDisconnected(const Arc::ClientID& clientId)
{
	Arc::LogInfo("Client disconnected! " + std::to_string(clientId));
}

void Application::OnDataReceived(const Arc::ClientID& clientId, const Arc::Buffer buffer)
{
	m_Server->SendBufferToAllClients(buffer, clientId);
}