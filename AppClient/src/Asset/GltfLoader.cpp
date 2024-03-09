#include "AssetCache.h"
#include "tinygltf/tiny_gltf.h"
#include "Core/Log.h"

void LoadMesh(const tinygltf::Model& input, const tinygltf::Node& node, std::vector<DynamicVertex>& vertices, std::vector<uint32_t>& indices)
{
	if (node.mesh > -1)
	{
		const tinygltf::Mesh m = input.meshes[node.mesh];

		for (size_t j = 0; j < m.primitives.size(); j++)
		{
			const tinygltf::Primitive& primitive = m.primitives[j];

			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;
			uint32_t startVertex = vertices.size();

			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const void* jointIndicesBuffer = nullptr;
				const float* jointWeightsBuffer = nullptr;

				int jointStride = 0;

				if (primitive.attributes.find("POSITION") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				// UVs
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					jointStride = accessor.ByteStride(view);
					jointIndicesBuffer = reinterpret_cast<const void*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					jointWeightsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

				}

				bool hasSkin = (jointIndicesBuffer && jointWeightsBuffer);

				vertices.reserve(vertexCount);
				for (size_t v = 0; v < vertexCount; v++) {
					DynamicVertex vert;
					vert.Position = glm::make_vec3(&positionBuffer[v * 3]);
					vert.Normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
					vert.TexCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
					vert.BoneIndices = glm::vec4(0.0f);
					if (hasSkin)
					{
						if (jointStride == 4)
							vert.BoneIndices = glm::vec4(glm::make_vec4(&((uint8_t*)jointIndicesBuffer)[v * 4]));
						if (jointStride == 8)
							vert.BoneIndices = glm::vec4(glm::make_vec4(&((uint16_t*)jointIndicesBuffer)[v * 4]));
					}
					vert.BoneWeights = hasSkin ? glm::make_vec4(&jointWeightsBuffer[v * 4]) : glm::vec4(1, 0, 0, 0);
					vertices.push_back(vert);
				}

			}

			if (primitive.indices > -1)
			{

				const tinygltf::Accessor& accessor = input.accessors[primitive.indices > -1 ? primitive.indices : 0];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				uint32_t startIndex = indices.size();
				indices.reserve(indexCount);

				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + startVertex);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + startVertex);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + startVertex);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return;
				}

				for (size_t i = startIndex; i < indices.size(); i += 3)
				{
					DynamicVertex& v1 = vertices[indices[i + 0]];
					DynamicVertex& v2 = vertices[indices[i + 1]];
					DynamicVertex& v3 = vertices[indices[i + 2]];

					glm::vec3 edge1 = v2.Position - v1.Position;
					glm::vec3 edge2 = v3.Position - v1.Position;
					glm::vec2 deltaUV1 = v2.TexCoord - v1.TexCoord;
					glm::vec2 deltaUV2 = v3.TexCoord - v1.TexCoord;

					float mu = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

					glm::vec3 tangent;
					tangent.x = mu * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
					tangent.y = mu * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
					tangent.z = mu * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

					v1.Tangent = tangent;
					v2.Tangent = tangent;
					v3.Tangent = tangent;
				}
			}
		}
	}

	for (auto& child : node.children)
	{
		const tinygltf::Node childNode = input.nodes[child];
		LoadMesh(input, childNode, vertices, indices);
	}

}

void LoadNodes(const tinygltf::Model& input, AnimationSet* animSet)
{
	animSet->nodes.resize(input.nodes.size());
	for (size_t i = 0; i < animSet->nodes.size(); i++)
	{
		auto& node = input.nodes[i];
		if (node.matrix.size() == 16)
			animSet->nodes[i].matrix = glm::make_mat4x4(node.matrix.data());

		for (auto& childIndex : node.children)
		{
			animSet->nodes[childIndex].parent = i;
			animSet->nodes[i].children.push_back(childIndex);
		}
	}
}

void LoadSkin(const tinygltf::Model& input, AnimationSet* animSet)
{
	if (input.skins.size() == 0)
		return;

	tinygltf::Skin glTFSkin = input.skins[0];

	animSet->skin.name = glTFSkin.name;
	animSet->skin.jointIndices.resize(glTFSkin.joints.size());

	for (int i = 0; i < glTFSkin.joints.size(); i++)
	{
		animSet->skin.jointIndices[i] = glTFSkin.joints[i];
	}

	if (glTFSkin.inverseBindMatrices > -1)
	{
		const tinygltf::Accessor& accessor = input.accessors[glTFSkin.inverseBindMatrices];
		const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
		animSet->skin.inverseBindMatrices.resize(accessor.count);
		memcpy(animSet->skin.inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
	}
}

void LoadAnimations(const tinygltf::Model& input, AnimationSet* animSet)
{
	//animSet->animations.resize(input.animations.size());
	
	for (size_t i = 0; i < input.animations.size(); i++)
	{
		tinygltf::Animation glTFAnimation = input.animations[i];


		auto& anim = animSet->animations[glTFAnimation.name];
		anim.name = glTFAnimation.name;

		// Samplers
		anim.samplers.resize(glTFAnimation.samplers.size());
		for (size_t j = 0; j < glTFAnimation.samplers.size(); j++)
		{
			tinygltf::AnimationSampler glTFSampler = glTFAnimation.samplers[j];
			AnimationSet::AnimationSampler& dstSampler = anim.samplers[j];

			// Read sampler keyframe input time values
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFSampler.input];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				const float* buf = static_cast<const float*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++)
				{
					dstSampler.inputs.push_back(buf[index]);
				}
				// Adjust animation's start and end times
				for (auto input : anim.samplers[j].inputs)
				{
					if (input < anim.start)
						anim.start = input;
					if (input > anim.end)
						anim.end = input;
				}
			}

			// Read sampler keyframe output translate/rotate/scale values
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFSampler.output];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				switch (accessor.type)
				{
				case TINYGLTF_TYPE_VEC3: {
					const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++)
					{
						dstSampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4: {
					const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++)
					{
						dstSampler.outputsVec4.push_back(buf[index]);
					}
					break;
				}
				default: {
					std::cout << "unknown type" << std::endl;
					break;
				}
				}
			}
		}

		// Channels
		anim.channels.resize(glTFAnimation.channels.size());

		//animationSet->Animations[i].
		//animationSet->Joints.resize(glTFAnimation.channels.size());
		for (size_t j = 0; j < glTFAnimation.channels.size(); j++)
		{
			tinygltf::AnimationChannel glTFChannel = glTFAnimation.channels[j];
			AnimationSet::AnimationChannel& dstChannel = anim.channels[j];
			dstChannel.path = glTFChannel.target_path;
			dstChannel.samplerIndex = glTFChannel.sampler;
			dstChannel.nodeIndex = glTFChannel.target_node;
		}
	}
}

glm::mat4 AnimationSet::GetNodeMatrix(int32_t nodeIndex)
{
	Node& node = nodes[nodeIndex];
	glm::mat4 nodeMatrix = glm::translate(glm::mat4(1.0f), node.translation) * glm::mat4(node.rotation) * glm::scale(glm::mat4(1.0f), node.scale) * node.matrix;
	int32_t currentParent = node.parent;
	while (currentParent != -1)
	{
		Node& parentNode = nodes[currentParent];
		nodeMatrix = glm::translate(glm::mat4(1.0f), parentNode.translation) * glm::mat4(parentNode.rotation) * glm::scale(glm::mat4(1.0f), parentNode.scale) * parentNode.matrix * nodeMatrix;
		currentParent = parentNode.parent;
	}
	return nodeMatrix;
}

void AnimationSet::SetGlobalMatrices(int32_t nodeIndex, glm::mat4& parentMatrix)
{
	Node& node = nodes[nodeIndex];
	glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), node.translation) * glm::mat4(node.rotation) * glm::scale(glm::mat4(1.0f), node.scale) * node.matrix;
	node.globalMatrix = parentMatrix * localMatrix;

	for (auto& childIndex : node.children)
	{
		SetGlobalMatrices(childIndex, node.globalMatrix);
	}
}

void AnimationSet::UpdateAnimation(std::string animationName, float time, std::vector<glm::mat4>& jointMatrices)
{

	if (!animations.contains(animationName))
	{
		jointMatrices.resize(100);
		for (size_t i = 0; i < 100; i++)
		{
			jointMatrices[i] = glm::mat4(1.0f);
		}
		return;
	}

	//int activeAnimation = animations.size() - 1;
	Animation& animation = animations[animationName];

	float currentTime = glm::mod(time, animation.end);
	for (auto& channel : animation.channels)
	{
		AnimationSampler& sampler = animation.samplers[channel.samplerIndex];

		for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
		{
			if ((currentTime >= sampler.inputs[i]) && (currentTime <= sampler.inputs[i + 1]))
			{
				float a = (currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
				if (channel.path == "translation")
				{
					nodes[channel.nodeIndex].translation = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
				}
				if (channel.path == "rotation")
				{
					glm::quat q1;
					q1.x = sampler.outputsVec4[i].x;
					q1.y = sampler.outputsVec4[i].y;
					q1.z = sampler.outputsVec4[i].z;
					q1.w = sampler.outputsVec4[i].w;

					glm::quat q2;
					q2.x = sampler.outputsVec4[i + 1].x;
					q2.y = sampler.outputsVec4[i + 1].y;
					q2.z = sampler.outputsVec4[i + 1].z;
					q2.w = sampler.outputsVec4[i + 1].w;

					nodes[channel.nodeIndex].rotation = glm::normalize(glm::slerp(q1, q2, a));
				}
				if (channel.path == "scale")
				{
					nodes[channel.nodeIndex].scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
				}
			}
		}
	}
	auto identity = glm::mat4(1.0f);
	SetGlobalMatrices(skin.jointIndices[0], identity);
	jointMatrices.resize(skin.jointIndices.size());
	for (size_t i = 0; i < skin.jointIndices.size(); i++)
	{
		Node& joint = nodes[skin.jointIndices[i]];
		//jointMatrices[i] = GetNodeMatrix(skin.jointIndices[i]) * skin.inverseBindMatrices[i];
		jointMatrices[i] = joint.globalMatrix * skin.inverseBindMatrices[i];
	}
}

void AssetCache::LoadGltf(Mesh* mesh, AnimationSet* animSet, std::string filePath)
{
	tinygltf::Model input;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	//bool fileLoaded = gltfContext.LoadASCIIFromFile(&input, &error, &warning, "res/Meshes/test.gltf");
	bool fileLoaded = gltfContext.LoadBinaryFromFile(&input, &error, &warning, filePath); // for binary glTF(.glb)

	if (fileLoaded)
	{
		std::vector<DynamicVertex> vertices;
		std::vector<uint32_t> indices;

		const tinygltf::Scene& scene = input.scenes[0];
		{
			// Root node
			// Currently supports only one root node
			const tinygltf::Node rootNode = input.nodes[scene.nodes[0]];
			LoadMesh(input, rootNode, vertices, indices);
			LoadNodes(input, animSet);
			LoadSkin(input, animSet);
			LoadAnimations(input, animSet);
		}

		mesh->VertexCount = vertices.size();
		mesh->IndexCount = indices.size();

		m_Device->GetResourceCache()->CreateBuffer(&mesh->VertexBuffer, Arc::GpuBufferDesc()
			.SetSize(vertices.size() * sizeof(DynamicVertex))
			.AddBufferUsage(Arc::BufferUsage::VertexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
			.AddMemoryPropertyFlag(Arc::MemoryProperty::DeviceLocal));
		m_Device->UploadToDeviceLocalBuffer(&mesh->VertexBuffer, vertices.data(), vertices.size() * sizeof(DynamicVertex));

		m_Device->GetResourceCache()->CreateBuffer(&mesh->IndexBuffer, Arc::GpuBufferDesc()
			.SetSize(indices.size() * sizeof(uint32_t))
			.AddBufferUsage(Arc::BufferUsage::IndexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
			.AddMemoryPropertyFlag(Arc::MemoryProperty::DeviceLocal));
		m_Device->UploadToDeviceLocalBuffer(&mesh->IndexBuffer, indices.data(), indices.size() * sizeof(uint32_t));

	}
	else
	{
		ARC_LOG("Gltf failed to load!");
	}
}