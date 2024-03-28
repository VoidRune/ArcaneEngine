#include "GuiBuilder.h"
#include "TextBuilder.h"
#include "Window/Input.h"
#include "Core/Log.h"

glm::vec2 s_Viewport;
std::vector<glm::vec4> s_ParentPointsStack;
TextBuilder s_TextBuilder;
std::vector<GuiVertex> s_GuiVertices;
std::vector<GuiVertex> s_GuiFontVertices;

void Gui::InitFont(const std::string& filePath)
{
	s_TextBuilder.LoadFont(filePath);
}

void Gui::Begin(int32_t viewportWidth, int32_t viewportHeight)
{
	s_GuiVertices.clear();
	s_GuiFontVertices.clear();
	s_Viewport = { viewportWidth, viewportHeight };
}

void Gui::End()
{

}

void Gui::BeginPanel(Constraint constraint)
{
	glm::vec2 TL = {0, 0};
	glm::vec2 BR = {0, 0};
	CalculateCornerPoints(TL, BR, constraint);

	s_ParentPointsStack.push_back(glm::vec4(TL.x, TL.y, BR.x - TL.x, BR.y - TL.y));
}

void Gui::BeginPanel(Constraint constraint, WidgetView color)
{
	glm::vec2 TL = { 0, 0 };
	glm::vec2 BR = { 0, 0 };
	CalculateCornerPoints(TL, BR, constraint);

	s_ParentPointsStack.push_back(glm::vec4(TL.x, TL.y, BR.x - TL.x, BR.y - TL.y));
	AddRectangle(TL, BR, color);
}

void Gui::EndPanel()
{
	s_ParentPointsStack.pop_back();
}

void Gui::Rectangle(Constraint constraint, WidgetView color)
{
	glm::vec2 TL = {0, 0};
	glm::vec2 BR = {0, 0};
	CalculateCornerPoints(TL, BR, constraint);

	AddRectangle(TL, BR, color);
}

bool Gui::Button(Constraint constraint, WidgetView defaultView, WidgetView hoverView)
{
	glm::vec2 TL = { 0, 0 };
	glm::vec2 BR = { 0, 0 };
	CalculateCornerPoints(TL, BR, constraint);

	glm::vec2 mouse = { Arc::Input::GetMouseX(), Arc::Input::GetMouseY() };
	bool hovering = glm::all(glm::greaterThan(mouse, TL) && glm::lessThan(mouse, BR));

	if(hovering)
		AddRectangle(TL, BR, hoverView);
	else
		AddRectangle(TL, BR, defaultView);

	return hovering && Arc::Input::IsKeyPressed(Arc::KeyCode::MouseLeft);
}

void Gui::Text(const std::string& text, TextDescription textDesc)
{
	if (!s_TextBuilder.IsLoaded())
	{
		ARC_LOG_WARNING("Gui::Text Font not loaded!");
		return;
	}

	TextBuilder::TextDescription desc;
	desc.ScreenPosition = { 100, 100 };
	desc.TopColor = textDesc.TopColor;
	desc.BottomColor = textDesc.BottomColor;
	desc.PixelHeight = textDesc.PixelHeight;
	desc.AllowedWidth = 800;

	s_TextBuilder.BuildText(text, desc, s_GuiFontVertices);
}

void Gui::Text(const std::string& text, Constraint constraint, TextDescription textDesc)
{
	if (!s_TextBuilder.IsLoaded())
	{
		ARC_LOG_WARNING("Gui::Text Font not loaded!");
		return;
	}

	glm::vec2 TL = { 0, 0 };
	glm::vec2 BR = { 0, 0 };
	CalculateCornerPoints(TL, BR, constraint);

	TextBuilder::TextDescription desc;
	desc.ScreenPosition = TL;
	desc.TopColor = textDesc.TopColor;
	desc.BottomColor = textDesc.BottomColor;
	desc.PixelHeight = textDesc.PixelHeight;
	desc.AllowedWidth = BR.x - TL.x;

	s_TextBuilder.BuildText(text, desc, s_GuiFontVertices);
}

void Gui::RenderGui(RenderFrameData& frameData)
{
	frameData.GuiVertexData = s_GuiVertices;
	frameData.GuiFontVertexData = s_GuiFontVertices;
}

void Gui::CalculateCornerPoints(glm::vec2& TL, glm::vec2& BR, Gui::Constraint c)
{
	TL = { 0, 0 };
	BR = { 0, 0 };
	glm::vec2 parentViewport = s_Viewport;
	if (s_ParentPointsStack.size() > 0)
	{
		glm::vec4 parent = s_ParentPointsStack[s_ParentPointsStack.size() - 1];
		TL = glm::vec2{ parent.x, parent.y };
		BR = TL;
		parentViewport = { parent.z, parent.w };
	}

	switch (c.PivotType)
	{
	case Pivot::Center:
		TL += c.PxCenter - c.PxDimensions * 0.5f + parentViewport * (c.PcCenter - c.PcDimensions * 0.5f);
		BR += c.PxCenter + c.PxDimensions * 0.5f + parentViewport * (c.PcCenter + c.PcDimensions * 0.5f);
		break;
	case Pivot::Left:
		TL.x += c.PxCenter.x + parentViewport.x * c.PcCenter.x;
		TL.y += c.PxCenter.y - c.PxDimensions.y * 0.5f + parentViewport.y * (c.PcCenter.y - c.PcDimensions.y * 0.5f);
		BR.x += c.PxCenter.x + c.PxDimensions.x + parentViewport.x * (c.PcCenter.x + c.PcDimensions.x);
		BR.y += c.PxCenter.y + c.PxDimensions.y * 0.5f + parentViewport.y * (c.PcCenter.y + c.PcDimensions.y * 0.5f);
		break;
	case Pivot::Top:
		TL.x += c.PxCenter.x - c.PxDimensions.x * 0.5f + parentViewport.x * (c.PcCenter.x - c.PcDimensions.x * 0.5f);
		TL.y += c.PxCenter.y + parentViewport.y * c.PcCenter.y;
		BR.x += c.PxCenter.x + c.PxDimensions.x * 0.5f + parentViewport.x * (c.PcCenter.x + c.PcDimensions.x * 0.5f);
		BR.y += c.PxCenter.y + c.PxDimensions.y + parentViewport.y * (c.PcCenter.y + c.PcDimensions.y);
		break;
	case Pivot::Right:
		TL.x += c.PxCenter.x - c.PxDimensions.x + parentViewport.x * (c.PcCenter.x - c.PcDimensions.x);
		TL.y += c.PxCenter.y - c.PxDimensions.y * 0.5f + parentViewport.y * (c.PcCenter.y - c.PcDimensions.y * 0.5f);
		BR.x += c.PxCenter.x + parentViewport.x * c.PcCenter.x;
		BR.y += c.PxCenter.y + c.PxDimensions.y * 0.5f + parentViewport.y * (c.PcCenter.y + c.PcDimensions.y * 0.5f);
		break;
	case Pivot::Bottom:
		TL.x += c.PxCenter.x - c.PxDimensions.x * 0.5f + parentViewport.x * (c.PcCenter.x - c.PcDimensions.x * 0.5f);
		TL.y += c.PxCenter.y - c.PxDimensions.y + parentViewport.y * (c.PcCenter.y - c.PcDimensions.y);
		BR.x += c.PxCenter.x + c.PxDimensions.x * 0.5f + parentViewport.x * (c.PcCenter.x + c.PcDimensions.x * 0.5f);
		BR.y += c.PxCenter.y + parentViewport.y * c.PcCenter.y;
		break;
	case Pivot::TopLeft:
		TL += c.PxCenter + parentViewport * c.PcCenter;
		BR += c.PxCenter + c.PxDimensions + parentViewport * (c.PcCenter + c.PcDimensions);
		break;
	case Pivot::TopRight:
		TL.x += c.PxCenter.x - c.PxDimensions.x + parentViewport.x * (c.PcCenter.x - c.PcDimensions.x);
		TL.y += c.PxCenter.y + parentViewport.y * c.PcCenter.y;
		BR.x += c.PxCenter.x + parentViewport.x * c.PcCenter.x;
		BR.y += c.PxCenter.y + c.PxDimensions.y + parentViewport.y * (c.PcCenter.y + c.PcDimensions.y);
		break;
	case Pivot::BottomRight:
		TL += c.PxCenter - c.PxDimensions + parentViewport * (c.PcCenter - c.PcDimensions);
		BR += c.PxCenter + parentViewport * c.PcCenter;
		break;
	case Pivot::BottomLeft:
		TL.x += c.PxCenter.x + parentViewport.x * c.PcCenter.x;
		TL.y += c.PxCenter.y - c.PxDimensions.y + parentViewport.y * (c.PcCenter.y - c.PcDimensions.y);
		BR.x += c.PxCenter.x + c.PxDimensions.x + parentViewport.x * (c.PcCenter.x + c.PcDimensions.x);
		BR.y += c.PxCenter.y + parentViewport.y * c.PcCenter.y;
		break;
	default:
		TL = { 0, 0 };
		BR = { 100, 100 };
	}
}

void Gui::AddRectangle(glm::vec2& TL, glm::vec2& BR, WidgetView color)
{
	s_GuiVertices.push_back({ TL, {0, 0}, color.Color });
	s_GuiVertices.push_back({ {BR.x, TL.y}, {1, 0}, color.Color });
	s_GuiVertices.push_back({ {TL.x, BR.y}, {0, 1}, color.Color });

	s_GuiVertices.push_back({ {TL.x, BR.y}, {0, 1}, color.Color });
	s_GuiVertices.push_back({ {BR.x, TL.y}, {1, 0}, color.Color });
	s_GuiVertices.push_back({ BR, {1, 1}, color.Color });
}