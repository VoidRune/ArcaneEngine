#pragma once
#include "Math/Math.h"
#include "VertexDefinitions.h"
#include "Asset/AssetCache.h"

struct StaticDrawData
{
	Model Model;
	uint32_t InstanceIndex = 0;
	uint32_t InstanceCount = 0;
};

struct DynamicDrawData
{
	Model Model;
	uint32_t InstanceIndex = 0;
	uint32_t InstanceCount = 0;

	uint32_t BoneDataStart = 0;
	uint32_t BoneDataCount = 0;
};

struct RenderFrameData
{
	glm::mat4 View;
	glm::mat4 Projection;
	glm::mat4 InvView;
	glm::mat4 InvProjection;
	glm::mat4 ShadowViewProj;
	glm::mat4 OrthoProjection;

	glm::vec3 CameraPosition;
	glm::vec3 SunShadowDirection;

	std::vector<glm::mat4> Transformations;
	std::vector<glm::mat4> BoneMatrices;
	std::vector<StaticDrawData> StaticDrawCalls;
	std::vector<DynamicDrawData> DynamicDrawCalls;

	std::vector<GuiVertex> GuiVertexData;
	std::vector<GuiVertex> GuiFontVertexData;
	uint32_t FontTextureBinding;

	void AddDynamicDrawData(Model model, const std::vector<glm::mat4>& transformations, const std::vector<glm::mat4>& boneMatrices)
	{
		DynamicDrawData drawCall;
		drawCall.Model = model;
		drawCall.InstanceIndex = Transformations.size();
		drawCall.InstanceCount = transformations.size();
		drawCall.BoneDataStart = BoneMatrices.size();
		drawCall.BoneDataCount = boneMatrices.size();

		for (auto& t : transformations)
			Transformations.push_back(t);
		for (auto& b : boneMatrices)
			BoneMatrices.push_back(b);

		DynamicDrawCalls.push_back(drawCall);
	}

	void AddStaticDrawData(Model model, glm::mat4 transformation)
	{
		StaticDrawData drawCall;
		drawCall.Model = model;
		drawCall.InstanceIndex = Transformations.size();
		drawCall.InstanceCount = 1;

		Transformations.push_back(transformation);

		StaticDrawCalls.push_back(drawCall);
	}

	void AddStaticDrawData(Model model, const std::vector<glm::mat4>& transformations)
	{
		StaticDrawData drawCall;
		drawCall.Model = model;
		drawCall.InstanceIndex = Transformations.size();
		drawCall.InstanceCount = transformations.size();

		for (auto& t : transformations)
			Transformations.push_back(t);

		StaticDrawCalls.push_back(drawCall);
	}

	void Clear()
	{
		StaticDrawCalls.clear();
		DynamicDrawCalls.clear();
		Transformations.clear();
		BoneMatrices.clear();
		GuiVertexData.clear();
	}
};