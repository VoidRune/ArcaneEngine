#pragma once
#include "Asset/AssetCache.h"
#include "Renderer/RenderFrameData.h"
#include "Math/QuadTree.h"


class World
{
public:
	World(AssetCache* assetCache);

	void RenderWorld(RenderFrameData& frameData);
	void Collide(glm::vec3& position, glm::vec3 velocity, float radius);

	class TileObject
	{
	public:
		Model Model;
		std::vector<Rectangle> Colliders;
	};

	const TileObject& GetTileObject(int32_t& index);
	void AddTile(int32_t index, glm::vec3 position, float angle = 0);

private:


	AssetCache* m_AssetCache;
	std::vector<std::vector<char>> m_Arena;

	class Tile
	{
	public:
		uint32_t TileIndex = 0;
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		float Angle = 0.0f;
	};

	// Handle to mesh, one per asset
	std::vector<TileObject> m_TileObjects;
	// World tile, multiple can reference a single TileObject
	std::vector<Tile> m_WorldTiles;

	StaticQuadTree<int32_t> m_QuadTree;
	std::vector<Rectangle> m_Test;

	Mesh m_WallMesh;
	Mesh m_FloorMesh;
	Mesh m_CubeMesh;

	Mesh m_SideMesh;
	Mesh m_CornerMesh;

	Mesh m_TileSet;

	Texture m_BaseColorTexture;
	Texture m_NormalTexture;
};