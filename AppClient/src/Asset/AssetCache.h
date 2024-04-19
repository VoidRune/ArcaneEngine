#pragma once
#include "Graphics/Device.h"
#include "Renderer/VertexDefinitions.h"
#include <string>
#include <unordered_map>


struct Mesh
{
	Arc::GpuBuffer VertexBuffer;
	Arc::GpuBuffer IndexBuffer;
	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
};

struct Texture
{
	Arc::GpuImage Image;
	uint32_t TextureBinding;
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

class AnimationSet
{
public:
	struct Node
	{
		int32_t parent = -1;
		std::vector<uint32_t> children;

		glm::vec3           translation{};
		glm::vec3           scale{ 1.0f };
		glm::quat           rotation{};

		glm::mat4 globalMatrix = glm::mat4(1.0f);
		glm::mat4 matrix = glm::mat4(1.0f);
	};

	struct Skin
	{
		std::string name;
		std::vector<uint32_t> jointIndices;
		std::vector<glm::mat4> inverseBindMatrices;
	};

	struct AnimationSampler
	{
		std::vector<float>     inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	struct AnimationChannel
	{
		std::string path;
		uint32_t nodeIndex;
		uint32_t samplerIndex;
	};

	struct Animation
	{
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
	};

	glm::mat4 GetNodeMatrix(int32_t nodeIndex);
	void SetGlobalMatrices(int32_t nodeIndex, glm::mat4& parentMatrix);
	void UpdateAnimation(std::string animationName, float time, std::vector<glm::mat4>& jointMatrices);

	std::vector<Node> nodes;
	Skin skin;
	std::unordered_map<std::string, Animation> animations;
};

class AssetCache
{
public:
	static void LoadObj(Mesh* mesh, std::string filePath);
	static void LoadImage(Texture* texture, std::string filePath);

	static void LoadGltf(Mesh* mesh, AnimationSet* animSet, std::string filePath);
	static void LoadGltf(Mesh* mesh, std::string filePath);

private:
	Arc::Device* m_Device;
};