#pragma once
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"

class TransferFunctionEditor
{
public:
	TransferFunctionEditor();
	~TransferFunctionEditor();

	void Render();
	std::vector<uint32_t> GenerateTransferFunctionImage(int width);

	struct Point
	{
		int32_t id;
		float X, Y;
		float R, G, B;
	};
private:

	void AddPoint();
	void RemoveSelectedPoint();
	void SortPoints();

	std::vector<Point> m_Points;
	int32_t m_SelectedPoint = -1;
	int32_t m_LastSelectedPoint = 0;
	int32_t m_CurrentPointId = 0;
};