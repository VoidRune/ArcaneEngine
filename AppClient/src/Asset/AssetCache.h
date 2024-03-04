#pragma once
#include "Graphics/Device.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

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

struct Mesh
{
	Arc::GpuBuffer VertexBuffer;
	Arc::GpuBuffer IndexBuffer;
	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
};

struct Texture
{
	Arc::Image Image;
	uint32_t ArrayIndex;
};

struct Model
{
	Model() { };

	Model(Mesh mesh, Texture baseColorTexture, Texture normalTexture)
	{
		Mesh = mesh;
		BaseColorTexture = baseColorTexture;
		NormalTexture = normalTexture;
	}

	Mesh Mesh;
	Texture BaseColorTexture;
	Texture NormalTexture;
};

class AssetCache
{
public:
	AssetCache(Arc::Device* device);
	~AssetCache();

	void LoadMesh(Mesh* mesh, std::string filePath);
	void LoadImage(Texture* texture, std::string filePath);

private:
	Arc::Device* m_Device;
	uint32_t m_BindlessTextureIndex = 0;
};