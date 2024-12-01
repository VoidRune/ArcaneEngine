#pragma once
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"
#include <functional>

class UserInterface
{
public:
	UserInterface();
	~UserInterface();

	void SetupDockspace();
	void EndDockspace();
	void RenderMenuBar(const std::vector<std::string>& files, std::function<void(std::string)>&& onSelect);
	void RenderCanvas(ImTextureID& displayImage, std::function<void(float, float)>&& func);
private:

};