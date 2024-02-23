#pragma once
#include "Graphics/Device.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

struct StaticVertex
{
	StaticVertex() { }

	StaticVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texCoord)
		: Position(position), Normal(normal), TexCoord(texCoord) { }

	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
};

struct Mesh
{
	Arc::GpuBuffer VertexBuffer;
	Arc::GpuBuffer IndexBuffer;
	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
};

struct Model
{
	Model(Mesh mesh)
	{
		Mesh = mesh;
		Transform = glm::mat4(1.0f);
	}

	Model(Mesh mesh, glm::vec3 translation)
	{
		Mesh = mesh;
		Transform = glm::translate(glm::mat4(1.0f), translation);
	}

	Mesh Mesh;
	glm::mat4 Transform;
};

class AssetCache
{
public:
	AssetCache(Arc::Device* device);
	~AssetCache();

	void LoadMesh(Mesh* mesh, std::string filePath);
private:
	Arc::Device* m_Device;
};