#pragma once
#include "Asset/AssetCache.h"
#include "Math/Shapes.h"
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

	std::vector<Rectangle> m_Test;

	Mesh m_WallMesh;
	Mesh m_FloorMesh;
	Mesh m_CubeMesh;

	Mesh m_SideMesh;
	Mesh m_CornerMesh;

	Texture m_BaseColorTexture;
	Texture m_NormalTexture;
};