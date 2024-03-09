#pragma once
#include <glm.hpp>

struct DrawCall
{
	Model Model;
	uint32_t InstanceIndex = 0;
	uint32_t InstanceCount = 0;
};

struct RenderFrameData
{
	glm::mat4 View;
	glm::mat4 Projection;
	glm::mat4 InvView;
	glm::mat4 InvProjection;
	std::vector<DrawCall> DrawCalls;
	std::vector<glm::mat4> Transformations;
	std::vector<glm::mat4> BoneMatrices;

	Model AnimatedModel;
	uint32_t AnimatedInstanceIndex = 0;

	void AddAnimatedModel(Model model, glm::mat4 transformation)
	{
		AnimatedModel = model;
		AnimatedInstanceIndex = Transformations.size();
		Transformations.push_back(transformation);
	}

	void AddDrawCall(Model model, glm::mat4 transformation)
	{
		DrawCall drawCall;
		drawCall.Model = model;
		drawCall.InstanceIndex = Transformations.size();
		drawCall.InstanceCount = 1;

		Transformations.push_back(transformation);

		DrawCalls.push_back(drawCall);
	}

	void AddDrawCall(Model model, const std::vector<glm::mat4>& transformations)
	{
		DrawCall drawCall;
		drawCall.Model = model;
		drawCall.InstanceIndex = Transformations.size();
		drawCall.InstanceCount = transformations.size();

		for (auto& t : transformations)
		{
			Transformations.push_back(t);
		}

		DrawCalls.push_back(drawCall);
	}

	void Clear()
	{
		DrawCalls.clear();
		Transformations.clear();
		BoneMatrices.clear();
	}
};