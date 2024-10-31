#pragma once
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"

class TransferFunctionEditor
{
public:
	TransferFunctionEditor();
	~TransferFunctionEditor();

	void Render();
	bool HasDataChanged() { return m_HasDataChanged; }
	std::vector<uint8_t> GenerateTransferFunctionImage(int width);

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
	bool m_HasDataChanged = false;
};