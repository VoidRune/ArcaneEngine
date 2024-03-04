#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RenderFrameData.h"
#include "Audio/AudioEngine.h"
#include "Networking/Client.h"
#include "World.h"
#include "PlayerController.h"

struct PlayerInfo
{
	glm::vec3 Position;
};

struct Projectile
{
	glm::vec3 Position;
	glm::vec3 Velocity;
	float LifeTime;
	bool Deadly;
};

class Game
{
public:
	Game(Arc::Window* window, AssetCache* assetCache);
	~Game();

	void Update(float deltaTime, RenderFrameData& frameData);

private:
	void OnConnected();
	void OnDisconnected();
	void OnDataReceived(const Arc::Buffer buffer);

private:
	Arc::Window* m_Window;
	AssetCache* m_AssetCache;
	std::unique_ptr<Arc::AudioEngine> m_AudioEngine;
	std::unique_ptr<Arc::AudioSource> m_ShootSound;

	Arc::Buffer m_ScratchBuffer;
	std::unique_ptr<Arc::Client> m_Client;

	std::unique_ptr<World> m_World;

	std::unique_ptr<PlayerController> m_PlayerController;
	std::unordered_map<uint32_t, PlayerInfo> m_Players;
	std::vector<Projectile> m_Projectiles;

	Mesh m_PlayerMesh;
	Mesh m_ProjectileMesh;
	Texture m_BaseColorTexture;
	Texture m_NormalTexture;
};