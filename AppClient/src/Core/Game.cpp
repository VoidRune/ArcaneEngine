#include "Game.h"
#include "Core/Log.h"
#include "Core/BufferStream.h"
#include "Core/PacketType.h"
#include "Gui/GuiBuilder.h"

Game::Game(Arc::Window* window, AssetCache* assetCache)
{
	m_Window = window;
	m_AssetCache = assetCache;
	m_ShootSound = std::make_unique<Arc::AudioSource>();
	Arc::AudioEngine::LoadFromFile(m_ShootSound.get(), "res/Audio/blaster_arcana_shoot.ogg");

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

	m_AssetCache->LoadObj(&m_ProjectileMesh, "res/Meshes/projectile.obj");
	m_AssetCache->LoadImage(&m_BaseColorTexture, "res/Textures/Sandstone/albedo.png");
	m_AssetCache->LoadImage(&m_NormalTexture, "res/Textures/Sandstone/normal.png");

	//m_AssetCache->LoadGltf(&m_FoxMesh, &gltfModel, "res/Meshes/character.glb");
	m_AssetCache->LoadGltf(&m_PlayerMesh, &animSet, "res/Meshes/test.glb");

	m_ScratchBuffer.Allocate(4096);

	Gui::InitFont("res/Fonts/fontmsdf.json");
	m_AssetCache->LoadImage(&m_FontTexture, "res/Fonts/fontmsdf.png");
}

Game::~Game()
{
	m_Client->Disconnect();
}

void Game::Update(float deltaTime, float elapsedTime, RenderFrameData& frameData)
{
	m_PlayerController->Update(deltaTime);
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

	frameData.Clear();
	frameData.View = m_PlayerController->GetCamera().View;
	frameData.Projection = m_PlayerController->GetCamera().Projection;
	frameData.InvView = m_PlayerController->GetCamera().InverseView;
	frameData.InvProjection = m_PlayerController->GetCamera().InverseProjection;

	glm::mat4 shadowOrtho = glm::ortho(-12.0f, 12.0f, -12.0f, 12.0f, 0.0f, 24.0f);
	shadowOrtho[1][1] *= -1;

	frameData.ShadowViewProj = shadowOrtho * glm::lookAt(m_PlayerController->GetPosition() + glm::vec3{ 2, 6, 1 }, m_PlayerController->GetPosition(), glm::vec3{0, 1, 0});
	//if (Arc::Input::IsKeyPressed(Arc::KeyCode::MouseLeft) && m_PlayerController->SpendMana(35.0f))
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


		glm::vec3 tilePos = glm::round(intersectionPoint);

		m_TileIndex += Arc::Input::GetScrollVertical();
		frameData.AddStaticDrawData(m_World->GetTileObject(m_TileIndex).Model, glm::translate(glm::mat4(1.0f), tilePos));
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::MouseRight))
		{
			m_World->AddTile(m_TileIndex, tilePos);
		}
		if (Arc::Input::IsKeyPressed(Arc::KeyCode::E))
		{
			m_World->AddTile(m_TileIndex, tilePos, 1.57079632679);
		}

		if (Arc::Input::IsKeyPressed(Arc::KeyCode::MouseLeft) && m_PlayerController->SpendMana(35.0f))
		{
			Projectile p;
			p.Position = m_PlayerController->GetPosition() + glm::vec3(0, 1, 0);
			p.Velocity = direction * 10.0f;
			p.LifeTime = 0.5f;
			p.Deadly = false;
			m_Projectiles.push_back(p);

			Arc::AudioEngine::Play(m_ShootSound.get());
			if (m_Client->GetConnectionStatus() == Arc::Client::ConnectionStatus::Connected)
			{
				Arc::BufferStreamWriter stream(m_ScratchBuffer);
				stream.WriteRaw<PacketType>(PacketType::ShootProjectile);
				stream.WriteRaw<uint32_t>(m_Client->GetClientID());
				stream.WriteRaw<Projectile>(p);
				m_Client->SendBuffer(Arc::Buffer(m_ScratchBuffer, stream.GetStreamPosition()), false);
			}
		}
	}

	std::vector<glm::mat4> playerPositions;
	for (auto& player : m_Players)
	{
		playerPositions.push_back(glm::translate(glm::mat4(1.0f), player.second.Position));
	}

	glm::mat4 playerTransform = glm::mat4(1.0f);
	playerTransform = glm::translate(playerTransform, m_PlayerController->GetPosition());
	playerTransform *= glm::mat4(m_PlayerController->GetRotation());

	std::vector<glm::mat4> tempBoneMatrices;
	if(m_PlayerController->HasMoved())
		animSet.UpdateAnimation("FastRun", elapsedTime, tempBoneMatrices);
	else
		animSet.UpdateAnimation("Idle", elapsedTime, tempBoneMatrices);

	frameData.AddDynamicDrawData(Model(m_PlayerMesh, m_BaseColorTexture, m_NormalTexture), { playerTransform, glm::mat4(1.0f)}, tempBoneMatrices);

	m_World->RenderWorld(frameData);

	std::vector<glm::mat4> projectilePositions;
	for (auto& projectile : m_Projectiles)
	{
		projectilePositions.push_back(glm::translate(glm::mat4(1.0f), projectile.Position));
	}
	frameData.AddStaticDrawData(Model(m_ProjectileMesh, m_BaseColorTexture, m_NormalTexture), projectilePositions);

	frameData.OrthoProjection = glm::ortho(0.0f, (float)m_Window->Width(), 0.0f, (float)m_Window->Height(), -10.0f, 10.0f);
	Gui::Begin(m_Window->Width(), m_Window->Height());
	
	Gui::WidgetView panelColor({ 0.2, 0.2, 0.2, 0.8 });
	Gui::Constraint con;
	con.PxCenter = { 10, 0 };
	con.PxDimensions = { 0, -20 };
	con.PcCenter = { 0, 0.5 };
	con.PcDimensions = { 0.25, 1 };
	con.PivotType = Gui::Pivot::Left;
	Gui::BeginPanel(con, panelColor);
	con.PxCenter = { 0, -10 };
	con.PxDimensions = { -20, 0 };
	con.PcCenter = { 0.5, 1 };
	con.PcDimensions = { 1, 0.2 };
	con.PivotType = Gui::Pivot::Bottom;
	Gui::WidgetView buttonColor({ 0, 1, 1, 1 });
	Gui::WidgetView hoverColor({ 1, 0, 0, 1 });
	if (Gui::Button(con, buttonColor, hoverColor))
	{
		ARC_LOG("Button!");
		m_Window->SetClosed(true);
	}
	con.PxCenter = { 10, 10 };
	con.PxDimensions = { -20, -20 };
	con.PcCenter = { 0, 0 };
	con.PcDimensions = { 1, 0.4 };
	con.PivotType = Gui::Pivot::TopLeft;
	Gui::TextDescription textDesc(24, false, { 0.169, 0.723, 0.94, 1 }, {0.075, 0.337, 0.95, 1});
	Gui::Text("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.", con, textDesc);
	Gui::EndPanel();
	Gui::RenderGui(frameData);
	frameData.FontTextureBinding = m_FontTexture.TextureBinding;
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