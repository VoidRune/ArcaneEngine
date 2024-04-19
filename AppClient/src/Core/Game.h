#pragma once
#include "Renderer/Renderer.h"
#include "Audio/AudioEngine.h"
#include "Networking/Client.h"
#include "World.h"
#include "PlayerController.h"
#include "Gui/TextBuilder.h"

struct PlayerInfo
{
	glm::vec3 Position;
};

class Game
{
public:
	Game(Arc::Window* window);
	~Game();

	void Update(float deltaTime, float elapsedTime, RenderFrameData& frameData);

private:
	void OnConnected();
	void OnDisconnected();
	void OnDataReceived(const Arc::Buffer buffer);

private:
	Arc::Window* m_Window;
	std::unique_ptr<Arc::AudioSource> m_ShootSound;

	Arc::Buffer m_ScratchBuffer;
	std::unique_ptr<Arc::Client> m_Client;

	int32_t m_TileIndex = 0;
	std::unique_ptr<World> m_World;

	std::unique_ptr<PlayerController> m_PlayerController;
	std::unordered_map<uint32_t, PlayerInfo> m_Players;
	std::vector<Projectile> m_Projectiles;


	Mesh m_PlayerMesh;
	AnimationSet animSet;

	float m_CurrentTileAngle = 0.0f;
	Mesh m_ProjectileMesh;
	Texture m_BaseColorTexture;
	Texture m_NormalTexture;
};