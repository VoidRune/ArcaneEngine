#include "World.h"
#include "Math/Collision.h"

World::World(AssetCache* assetCache)
{
	m_AssetCache = assetCache;

	m_AssetCache->LoadObj(&m_FloorMesh, "res/Meshes/floor.obj");
	m_AssetCache->LoadObj(&m_WallMesh, "res/Meshes/fence.obj");
	m_AssetCache->LoadObj(&m_SideMesh, "res/Meshes/side.obj");
	m_AssetCache->LoadObj(&m_CornerMesh, "res/Meshes/corner.obj");

	m_AssetCache->LoadImage(&m_BaseColorTexture, "res/Textures/Greystone/albedo.png");
	m_AssetCache->LoadImage(&m_NormalTexture, "res/Textures/Greystone/normal.png");

	m_AssetCache->LoadObj(&m_CubeMesh, "res/Meshes/cube.obj");

    m_Arena = 
    {
        {'a','_','_','_','_','_','_','d'},
        {'|',' ',' ',' ',' ',' ',' ','/'},
        {'|',' ',' ',' ',' ',' ',' ','/'},
        {'|',' ',' ',' ',' ',' ',' ','/'},
        {'|',' ',' ',' ',' ',' ',' ','/'},
        {'|',' ',' ',' ',' ',' ',' ','/'},
        {'|',' ',' ',' ',' ',' ',' ','/'},
        {'b','-','-','-','-','-','-','c'},
    };


	Rectangle r;
	r.Position = { 5, 2 };
	r.Width = 2.0f;
	r.Height = 1.0f;
	r.Angle = 1.0f;
	m_Test.push_back(r);
}

void World::Collide(glm::vec3& position, glm::vec3 velocity, float radius)
{
	glm::vec2 playerPos = glm::vec2(position.x, position.z);
	glm::vec2 potentialPosition = playerPos + glm::vec2(velocity.x, velocity.z);

	glm::vec2 vCurrentCell = glm::floor(playerPos);
	glm::vec2 vTargetCell = glm::floor(potentialPosition);
	glm::vec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::vec2(1, 1), glm::vec2(0.0));
	glm::vec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::vec2(1, 1), glm::vec2(7, 7));

	float playerRadius = radius;
	glm::vec2 vCell;

	Circle c;
	c.Position = potentialPosition;
	c.Radius = radius;

	for (vCell.y = vAreaTL.y; vCell.y <= vAreaBR.y; vCell.y++)
	{
		for (vCell.x = vAreaTL.x; vCell.x <= vAreaBR.x; vCell.x++)
		{
			if (m_Arena[(int)vCell.x][(int)vCell.y] == '#')
			{
				//RectangleCircleCollision(vCell, potentialPosition, radius);

				//glm::vec2 vNearestPoint;
				//vNearestPoint.x = std::max(float(vCell.x + TL), std::min(potentialPosition.x, float(vCell.x + BR)));
				//vNearestPoint.y = std::max(float(vCell.y + TL), std::min(potentialPosition.y, float(vCell.y + BR)));
				//glm::vec2 vRayToNearest = vNearestPoint - potentialPosition;
				//float fOverlap = playerRadius - glm::length(vRayToNearest);
				//
				//if (glm::isnan(fOverlap)) fOverlap = 0.0f;
				//
				//if (fOverlap > 0.0f && vRayToNearest != glm::vec2(0.0f, 0.0f))
				//{
				//	potentialPosition = potentialPosition - glm::normalize(vRayToNearest) * fOverlap;
				//}
			}
		}
	}

	for (auto& rect : m_Test)
	{
		CollisionResolution(c, rect);
	}

	position.x = c.Position.x;
	position.z = c.Position.y;
}

void World::RenderWorld(RenderFrameData& frameData)
{
	std::vector<glm::mat4> wallPositions;
	std::vector<glm::mat4> cornerPositions;
	std::vector<glm::mat4> floorPositions;
	for (size_t z = 0; z < m_Arena.size(); z++)
	{
		for (size_t x = 0; x < m_Arena[z].size(); x++)
		{
			float xPos = 2.0f * x;
			float zPos = 2.0f * z;
            char c = m_Arena[z][x];
            switch (c)
            {
            case '|':
				wallPositions.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)));
                break;
			case '-':
				wallPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 1.57079f, glm::vec3(0, 1, 0)));
				break;
			case '/':
				wallPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 3.1415f, glm::vec3(0, 1, 0)));
				break;
			case '_':
				wallPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 4.71238f, glm::vec3(0, 1, 0)));
				break;
			case 'a':
				cornerPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 0.0f, glm::vec3(0, 1, 0)));
				break;
			case 'b':
				cornerPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 1.57079f, glm::vec3(0, 1, 0)));
				break;
			case 'c':
				cornerPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 3.1415f, glm::vec3(0, 1, 0)));
				break;
			case 'd':
				cornerPositions.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)), 4.71238f, glm::vec3(0, 1, 0)));
				break;
            case ' ':
				floorPositions.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0, zPos)));
                break;
            }
		}
	}
	frameData.AddDrawCall(Model(m_SideMesh, m_BaseColorTexture, m_NormalTexture), wallPositions);
	frameData.AddDrawCall(Model(m_CornerMesh, m_BaseColorTexture, m_NormalTexture), cornerPositions);
	frameData.AddDrawCall(Model(m_FloorMesh, m_BaseColorTexture, m_NormalTexture), floorPositions);

	std::vector<glm::mat4> cubePositions;
	for (auto& rect : m_Test)
	{
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, glm::vec3(rect.Position.x, 0.0f, rect.Position.y));
		transform = glm::rotate(transform, rect.Angle, glm::vec3(0, 1, 0));
		transform = glm::scale(transform, glm::vec3(rect.Width, 1.0f, rect.Height));
		cubePositions.push_back(transform);
	}
	frameData.AddDrawCall(Model(m_CubeMesh, m_BaseColorTexture, m_NormalTexture), cubePositions);

}