#pragma once
#include <memory>
#include "Networking/Client.h"

class Application
{
public:
	Application();
	~Application();

	void Update();
	bool IsRunning();

private:
	void OnConnected();
	void OnDisconnected();
	void OnDataReceived(const Arc::Buffer buffer);

private:
	std::unique_ptr<Arc::Client> m_Client;

};