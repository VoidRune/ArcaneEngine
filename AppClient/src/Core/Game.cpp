#include "Game.h"
#include "Core/Log.h"
#include "Core/BufferStream.h"
#include "Core/PacketType.h"

Game::Game(Arc::Window* window, AssetCache* assetCache)
{
	m_Window = window;
	m_AssetCache = assetCache;
	m_AudioEngine = std::make_unique<Arc::AudioEngine>();
	m_ShootSound = std::make_unique<Arc::AudioSource>();
	m_AudioEngine->LoadFromFile(m_ShootSound.get(), "res/Audio/blaster_arcana_shoot.ogg");

	std::string ip = "127.0.0.1:60000";

	m_Client = std::make_unique<Arc::Client>();
	m_Client->SetServerConnectedCallback([this]() { OnConnected(); });
	m_Client->SetServerDisconnectedCallback([this]() { OnDisconnected(); });
	m_Client->SetDataReceivedCallback([this](const Arc::Buffer data) { OnDataReceived(data); });
	m_Client->ConnectToServer(ip.c_str());

	m_World = std::make_unique<World>(m_AssetCache);
	m_PlayerController = std::make_unique<PlayerController>(m_Window);
	m_PlayerController->SetPosition({3, 0, 3});
	m_PlayerController->SetMovementSpeed(4.0f);

	m_AssetCache->LoadMesh(&m_PlayerMesh, "res/Meshes/cylinder.obj");
	m_AssetCache->LoadMesh(&m_ProjectileMesh, "res/Meshes/projectile.obj");
	m_AssetCache->LoadImage(&m_BaseColorTexture, "res/Textures/Sandstone/albedo.png");
	m_AssetCache->LoadImage(&m_NormalTexture, "res/Textures/Sandstone/normal.png");

	m_ScratchBuffer.Allocate(4096);
}

Game::~Game()
{
	m_Client->Disconnect();
}

void Game::Update(float deltaTime, RenderFrameData& frameData)
{
	m_PlayerController->UpdateVelocity(deltaTime);
	m_World->Collide(m_PlayerController->GetPosition(), m_PlayerController->GetVelocity() * deltaTime, 0.4f);
	m_PlayerController->UpdateCamera();

	if (m_Client->GetConnectionStatus() == Arc::Client::ConnectionStatus::Connected && m_PlayerController->HasMoved())
	{
		Arc::BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw<PacketType>(PacketType::MovePlayer);
		stream.WriteRaw<uint32_t>(m_Client->GetClientID());
		stream.WriteRaw<PlayerInfo>(PlayerInfo(m_PlayerController->GetPosition()));
		m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()), false);
	}

	std::vector<Projectile> newProjectiles;
	for (auto& projectile : m_Projectiles)
	{
		bool remove = false;
		projectile.Position += projectile.Velocity * deltaTime;
		projectile.LifeTime -= deltaTime;
		if (projectile.LifeTime <= 0.0f)
			remove = true;
		if (projectile.Deadly)
		{
			float x = m_PlayerController->GetPosition().x - projectile.Position.x;
			float z = m_PlayerController->GetPosition().z - projectile.Position.z;
			if (x * x + z * z <= 0.16)
			{
				ARC_LOG("Hit!");
				remove = true;
				m_PlayerController->SetPosition({ 3, 0, 3 });
			}
		}

		if(!remove)
			newProjectiles.push_back(projectile);
	}
	m_Projectiles = newProjectiles;

	if (Arc::Input::IsKeyPressed(Arc::KeyCode::MouseLeft))
	{
		glm::vec3 rayDirection = m_PlayerController->GetCamera().ProjectRay(Arc::Input::GetMouseX() / (float)m_Window->Width(), Arc::Input::GetMouseY() / (float)m_Window->Height());

		float t = 0;
		float denom = glm::dot(rayDirection, glm::vec3(0, 1, 0));
		if (glm::abs(denom) > 0.0001f)
		{
			t = (glm::vec3(0, 0, 0) - m_PlayerController->GetCamera().Position).y / denom;
		}

		glm::vec3 intersectionPoint = m_PlayerController->GetCamera().Position + rayDirection * t;
		glm::vec3 direction = glm::normalize(intersectionPoint - m_PlayerController->GetPosition());

		Projectile p;
		p.Position = m_PlayerController->GetPosition() + glm::vec3(0, 1, 0);
		p.Velocity = direction * 10.0f;
		p.LifeTime = 0.5f;
		p.Deadly = false;
		m_Projectiles.push_back(p);

		m_AudioEngine->Play(m_ShootSound.get());
		if (m_Client->GetConnectionStatus() == Arc::Client::ConnectionStatus::Connected)
		{
			Arc::BufferStreamWriter stream(m_ScratchBuffer);
			stream.WriteRaw<PacketType>(PacketType::ShootProjectile);
			stream.WriteRaw<uint32_t>(m_Client->GetClientID());
			stream.WriteRaw<Projectile>(p);
			m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()), false);
		}
	}

	frameData.Clear();
	frameData.View = m_PlayerController->GetCamera().View;
	frameData.Projection = m_PlayerController->GetCamera().Projection;
	frameData.InvView = m_PlayerController->GetCamera().InverseView;
	frameData.InvProjection = m_PlayerController->GetCamera().InverseProjection;

	std::vector<glm::mat4> playerPositions;
	for (auto& player : m_Players)
	{
		playerPositions.push_back(glm::translate(glm::mat4(1.0f), player.second.Position));
	}
	playerPositions.push_back(glm::translate(glm::mat4(1.0f), m_PlayerController->GetPosition()));
	frameData.AddDrawCall(Model(m_PlayerMesh, m_BaseColorTexture, m_NormalTexture), playerPositions);

	m_World->RenderWorld(frameData);

	std::vector<glm::mat4> projectilePositions;
	for (auto& projectile : m_Projectiles)
	{
		projectilePositions.push_back(glm::translate(glm::mat4(1.0f), projectile.Position));
	}
	frameData.AddDrawCall(Model(m_ProjectileMesh, m_BaseColorTexture, m_NormalTexture), projectilePositions);
}


void Game::OnConnected()
{
	Arc::LogInfo("Server connected!");

	Arc::BufferStreamWriter stream(m_ScratchBuffer);
	stream.WriteRaw<PacketType>(PacketType::RequestPlayerInfo);
	stream.WriteRaw<uint32_t>(m_Client->GetClientID());
	m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
}

void Game::OnDisconnected()
{
	Arc::LogInfo("Server disconnected!");
}

void Game::OnDataReceived(const Arc::Buffer buffer)
{
	Arc::BufferStreamReader stream(buffer);
	PacketType type;
	stream.ReadRaw<PacketType>(type);
	uint32_t senderId;
	stream.ReadRaw<uint32_t>(senderId);

	switch (type)
	{
	case PacketType::AddPlayer:
	{
		PlayerInfo playerInfo;
		stream.ReadRaw<PlayerInfo>(playerInfo);
		m_Players[senderId] = playerInfo;

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

		break;
	}
	case PacketType::RequestPlayerInfo:
	{
		Arc::BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw<PacketType>(PacketType::AddPlayer);
		stream.WriteRaw<uint32_t>(m_Client->GetClientID());
		stream.WriteRaw<PlayerInfo>(PlayerInfo(m_PlayerController->GetPosition()));
		m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()));

		break;
	}
	case PacketType::ShootProjectile:
	{
		Projectile projectile;
		stream.ReadRaw<Projectile>(projectile);
		projectile.Deadly = true;
		m_Projectiles.push_back(projectile);

		break;
	}
	}
}