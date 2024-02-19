#pragma once
#include <memory>
#include "Networking/Server.h"

class Application
{
public:
	Application();
	~Application();

	void Update();
	bool IsRunning();

private:
	void OnClientConnected(const Arc::ClientID& clientId);
	void OnClientDisconnected(const Arc::ClientID& clientId);
	void OnDataReceived(const Arc::ClientID& clientId, const Arc::Buffer buffer);

private:
	std::unique_ptr<Arc::Server> m_Server;

};