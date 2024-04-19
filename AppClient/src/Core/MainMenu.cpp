#include "MainMenu.h"
#include "Gui/GuiBuilder.h"
#include "Math/Math.h"
#include "Core/Log.h"

MainMenu::MainMenu(Arc::Window* window)
{
	m_Window = window;

	AssetCache::LoadImage(&m_LogoTexture, "res/Textures/Logo.png");
	AssetCache::LoadImage(&m_BackgroundTexture, "res/Textures/Background.jpg");

}

MainMenu::~MainMenu()
{

}

void MainMenu::Update(float deltaTime, float elapsedTime, RenderFrameData& frameData)
{
	frameData.OrthoProjection = glm::ortho(0.0f, (float)m_Window->Width(), 0.0f, (float)m_Window->Height(), -10.0f, 10.0f);
	Gui::Begin(m_Window->Width(), m_Window->Height());

	Gui::Constraint con;
	con.PxPivotCenter = { 0, 0 };
	con.PxDimensions = { 0, 0 };
	con.PcPivotCenter = { 0.5, 0 };
	con.PcDimensions = { 1, 0 };
	con.HeightFromAspect = 0.5626;
	con.PivotType = Gui::Pivot::Top;
	Gui::Rectangle(con, Gui::WidgetView{ {1, 1, 1, 1}, m_BackgroundTexture.TextureBinding });
	con.PxPivotCenter = { 0, 0 };
	con.PxDimensions = { 0, 0 };
	con.PcPivotCenter = { 0.5, 0.1 };
	con.PcDimensions = { 0.45, 0 };
	con.HeightFromAspect = 0.2125;
	con.PivotType = Gui::Pivot::Top;
	Gui::Rectangle(con, Gui::WidgetView{ {1, 1, 1, 1}, m_LogoTexture.TextureBinding });

	con.PxPivotCenter = { 0, 0 };
	con.PxDimensions = { 0, 0 };
	con.PcPivotCenter = { 0.5, 0.6 };
	con.PcDimensions = { 0.3, 0.05 };
	con.HeightFromAspect = 0.0f;
	con.PivotType = Gui::Pivot::Center;

	Gui::WidgetView buttonColor({ 0.2, 0.2, 0.2, 0.2 });
	Gui::WidgetView hoverColor({ 0.2, 0.2, 0.2, 0.4 });
	if (Gui::Button(con, buttonColor, hoverColor))
	{
		GlobalGameData::CurrentGameState = GameState::Game;
	}

	con.PcPivotCenter = { 0.5, 0.67 };
	if (Gui::Button(con, buttonColor, hoverColor))
	{
		ARC_LOG("Button!");
		m_Window->SetClosed(true);
	}

	con.PcPivotCenter = { 0.5, 0.74 };
	if (Gui::Button(con, buttonColor, hoverColor))
	{
		ARC_LOG("Button!");
		m_Window->SetClosed(true);
	}

	Gui::RenderGui(frameData);
}
