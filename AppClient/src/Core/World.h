#pragma once
#include "Asset/AssetCache.h"
#include "Renderer/RenderFrameData.h"

class World
{
public:
	World(AssetCache* assetCache);

	void RenderWorld(RenderFrameData& frameData);
	void Collide(glm::vec3& position, glm::vec3 velocity, float radius);

private:
	AssetCache* m_AssetCache;
	std::vector<std::vector<char>> m_Arena;

	Mesh m_WallMesh;
	Mesh m_FloorMesh;
	Texture m_BaseColorTexture;
	Texture m_NormalTexture;
};