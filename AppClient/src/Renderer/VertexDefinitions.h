#pragma once
#include "Math/Math.h"

struct StaticVertex
{
	StaticVertex() { }

	StaticVertex(glm::vec3 position, glm::vec3 normal, glm::vec3 tangent, glm::vec2 texCoord)
		: Position(position), Normal(normal), Tangent(tangent), TexCoord(texCoord) { }

	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec2 TexCoord;
};

struct DynamicVertex
{
	DynamicVertex() { }

	DynamicVertex(glm::vec3 position, glm::vec3 normal, glm::vec3 tangent, glm::vec2 texCoord)
		: Position(position), Normal(normal), Tangent(tangent), TexCoord(texCoord) { }

	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec2 TexCoord;
	glm::vec4 BoneIndices;
	glm::vec4 BoneWeights;
};

struct GuiVertex
{
	GuiVertex() { }

	GuiVertex(glm::vec2 position, glm::vec2 texCoord, glm::vec4 color, uint32_t textureIndex)
		: Position(position), TexCoord(texCoord), Color(color), TextureIndex(textureIndex) { }

	glm::vec2 Position;
	glm::vec2 TexCoord;
	glm::vec4 Color;
	uint32_t TextureIndex;
};