#include "World.h"

World::World(AssetCache* assetCache)
{
	m_AssetCache = assetCache;

	m_AssetCache->LoadMesh(&m_FloorMesh, "res/Meshes/floor.obj");
	m_AssetCache->LoadMesh(&m_WallMesh, "res/Meshes/wall.obj");
	m_AssetCache->LoadImage(&m_BaseColorTexture, "res/Textures/Greystone/albedo.png");
	m_AssetCache->LoadImage(&m_NormalTexture, "res/Textures/Greystone/normal.png");

    m_Arena = 
    {
        { 'V','V','V','#','#','#','#','#','#','#','#','#','#','#','V','V','V' },
        { 'V','V','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#','V','V' },
        { 'V','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#','V' },
        { '#',' ',' ',' ','#',' ',' ','#',' ','#',' ',' ','#',' ',' ',' ','#' },
        { '#',' ',' ','#','#',' ',' ',' ',' ',' ',' ',' ','#','#',' ',' ','#' },
        { '#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#' },
        { '#',' ',' ',' ',' ',' ','#','#','#','#','#',' ',' ',' ',' ',' ','#' },
        { '#',' ',' ','#',' ',' ','#','V','V','V','#',' ',' ','#',' ',' ','#' },
        { '#',' ',' ',' ',' ',' ','#','V','V','V','#',' ',' ',' ',' ',' ','#' },
        { '#',' ',' ','#',' ',' ','#','V','V','V','#',' ',' ','#',' ',' ','#' },
        { '#',' ',' ',' ',' ',' ','#','#','#','#','#',' ',' ',' ',' ',' ','#' },
        { '#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#' },
        { '#',' ',' ','#','#',' ',' ',' ',' ',' ',' ',' ','#','#',' ',' ','#' },
        { '#',' ',' ',' ','#',' ',' ','#',' ','#',' ',' ','#',' ',' ',' ','#' },
        { 'V','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#','V' },
        { 'V','V','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#','V','V' },
        { 'V','V','V','#','#','#','#','#','#','#','#','#','#','#','V','V','V' }
    };
}

void World::Collide(glm::vec3& position, glm::vec3 velocity, float radius)
{
	glm::vec2 playerPos = glm::vec2(position.x, position.z);
	glm::vec2 potentialPosition = playerPos + glm::vec2(velocity.x, velocity.z);

	glm::vec2 vCurrentCell = glm::floor(playerPos);
	glm::vec2 vTargetCell = glm::floor(potentialPosition);
	glm::vec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::vec2(1, 1), glm::vec2(0.0));
	glm::vec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::vec2(1, 1), glm::vec2(16, 16));

	float playerRadius = radius;
	glm::vec2 vCell;
	float TL = 0.0f;
	float BR = 1.0f;

	for (vCell.y = vAreaTL.y; vCell.y <= vAreaBR.y; vCell.y++)
	{
		for (vCell.x = vAreaTL.x; vCell.x <= vAreaBR.x; vCell.x++)
		{
			if (m_Arena[(int)vCell.x][(int)vCell.y] == '#')
			{
				glm::vec2 vNearestPoint;
				vNearestPoint.x = std::max(float(vCell.x + TL), std::min(potentialPosition.x, float(vCell.x + BR)));
				vNearestPoint.y = std::max(float(vCell.y + TL), std::min(potentialPosition.y, float(vCell.y + BR)));
				glm::vec2 vRayToNearest = vNearestPoint - potentialPosition;
				float fOverlap = playerRadius - glm::length(vRayToNearest);

				if (glm::isnan(fOverlap)) fOverlap = 0.0f;

				if (fOverlap > 0.0f && vRayToNearest != glm::vec2(0.0f, 0.0f))
				{
					potentialPosition = potentialPosition - glm::normalize(vRayToNearest) * fOverlap;
				}
			}
		}
	}

	position.x = potentialPosition.x;
	position.z = potentialPosition.y;
}

void World::RenderWorld(RenderFrameData& frameData)
{
	std::vector<glm::mat4> wallPositions;
	std::vector<glm::mat4> floorPositions;
	for (size_t z = 0; z < m_Arena.size(); z++)
	{
		for (size_t x = 0; x < m_Arena[z].size(); x++)
		{
            char c = m_Arena[z][x];
            switch (c)
            {
            case '#':
				wallPositions.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(x, 0, z)));
                break;
            case ' ':
				floorPositions.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(x, 0, z)));
                break;
            }
		}
	}
	frameData.AddDrawCall(Model(m_WallMesh, m_BaseColorTexture, m_NormalTexture), wallPositions);
	frameData.AddDrawCall(Model(m_FloorMesh, m_BaseColorTexture, m_NormalTexture), floorPositions);
}