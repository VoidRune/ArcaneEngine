#pragma once
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"
#include <functional>

class UserInterface
{
public:
	UserInterface();
	~UserInterface();

	struct SliderInt
	{
		const char* Name;
		int* Value;
		int Min;
		int Max;
	};
	struct SliderFloat
	{
		const char* Name;
		float* Value;
		float Min;
		float Max;
	};
	struct ColorEdit
	{
		const char* Name;
		float* Value;
	};

	void SetupDockspace();
	void EndDockspace();
	void RenderMenuBar(const std::vector<std::string>& files, std::function<void(std::string)>&& onSelect);
	void RenderCanvas(ImTextureID& displayImage, std::function<void(float, float)>&& func);
	bool RenderSettings(SliderInt bounceLimit,
						SliderFloat extinction,
						SliderFloat anisotropy,
						ColorEdit backgroundColor);
	bool RenderDebugSettings(int* enableDebugDraw, int* maxScatteringEvent, int* maxNullEvent);
private:

};