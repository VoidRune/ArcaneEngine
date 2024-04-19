#pragma once
#include "GlobalGameData.h"
#include "Renderer/Renderer.h"

class MainMenu
{
public:
	MainMenu(Arc::Window* window);
	~MainMenu();

	void Update(float deltaTime, float elapsedTime, RenderFrameData& frameData);

private:
	Arc::Window* m_Window;

	Texture m_LogoTexture;
	Texture m_BackgroundTexture;

};