#pragma once
#include "Renderer/Renderer.h"

class Gui
{
public:

	enum class Pivot
	{
		Center = 0,
		Left,
		Top,
		Right,
		Bottom,
		TopLeft,
		TopRight,
		BottomRight,
		BottomLeft
	};

	/*         X                     */
	/* [0, 0]  ----->         [1, 0] */
	/*        *-------------*        */
	/*    Y | |             |        */
	/*      | |             |        */
	/*      V |             |        */
	/*        *-------------*        */
	/* [0, 1]                 [1, 1] */
						    
	class Constraint
	{
	public:
		// Pixel values
		glm::vec2 PxDimensions{};
		glm::vec2 PxPivotCenter{};

		// Percent values
		glm::vec2 PcDimensions{};
		glm::vec2 PcPivotCenter{};

		// Set Width based on Height
		// If set to 2.5, then width will be 2.5 times the height
		// Overrides other Width constraints
		float WidthFromAspect = 0.0f;
		// Set Height based on Width
		// If set to 2.5, then height will be 2.5 times the width
		// Overrides other Height constraints
		float HeightFromAspect = 0.0f;

		Pivot PivotType = Pivot::Center;
	};

	class WidgetView
	{
	public:
		glm::vec4 Color;
		uint32_t TextureIndex = Renderer::GetWhiteTexture();
	};

	class TextDescription
	{
	public:
		int32_t PixelHeight = 40;
		bool Centered = false;
		glm::vec4 TopColor;
		glm::vec4 BottomColor;
	};

	static void InitFont(const std::string& filePath);

	static void Begin(int32_t viewportWidth, int32_t viewportHeight);
	static void End();

	static void BeginPanel(Constraint constraint);
	static void BeginPanel(Constraint constraint, WidgetView color);
	static void EndPanel();

	static void Rectangle(Constraint constraint, WidgetView color);
	static bool Button(Constraint constraint, WidgetView defaultView, WidgetView hoverView);
	static void Text(const std::string& text, TextDescription textDesc);
	static void Text(const std::string& text, Constraint constraint, TextDescription textDesc);

	static void RenderGui(RenderFrameData& frameData);

private:
	static void CalculateCornerPoints(glm::vec2& TL, glm::vec2& BR, Constraint constraint);
	static void AddRectangle(glm::vec2& TL, glm::vec2& BR, WidgetView color);

};