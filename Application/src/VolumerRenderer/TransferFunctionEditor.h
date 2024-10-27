#pragma once
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"

class TransferFunctionEditor
{
public:
	TransferFunctionEditor();
	~TransferFunctionEditor();

	void Render();

	struct Point
	{
		int32_t id;
		float X, Y;
		float R, G, B;
	};
private:

	void AddPoint();
	void SortPoints();

	std::vector<Point> m_Points;
	int32_t m_SelectedPoint = -1;
	int32_t m_LastSelectedPoint = 0;
	int32_t m_CurrentPointId = 1;
};