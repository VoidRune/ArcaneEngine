#pragma once
#include <memory>
#include "Renderer/Renderer.h"
#include "Networking/Client.h"
#include "CameraFPS.h"

struct PlayerInfo
{
	glm::vec3 Position;
};

class Application
{
public:
	Application();
	~Application();

	void Run();

private:
	void OnConnected();
	void OnDisconnected();
	void OnDataReceived(const Arc::Buffer buffer);

private:
	std::unique_ptr<Arc::Window> m_Window;
	std::unique_ptr<Arc::Device> m_Device;
	std::unique_ptr<Arc::PresentQueue> m_PresentQueue;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<AssetCache> m_AssetCache;

	Arc::Buffer m_ScratchBuffer;
	std::unique_ptr<Arc::Client> m_Client;

	float m_LastTime;
	std::unique_ptr<CameraFPS> m_Camera;
	std::unordered_map<uint32_t, PlayerInfo> m_Players;

	Mesh m_Mesh;
};