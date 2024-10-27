#include "TransferFunctionEditor.h"
#include <algorithm>

TransferFunctionEditor::TransferFunctionEditor()
{
	m_Points.push_back(Point(m_CurrentPointId++, 0.0, 0.0, 255, 0, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 0.2, 0.6, 255, 255, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 0.4, 0.1, 0, 255, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 0.7, 0.2, 255, 0, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 1.0, 0.0, 255, 0, 0));
}

TransferFunctionEditor::~TransferFunctionEditor()
{

}

void TransferFunctionEditor::AddPoint()
{
	m_Points.push_back(Point(m_CurrentPointId++, 0.5, 0.5, 255, 0, 0));
}

void TransferFunctionEditor::SortPoints()
{
	std::sort(m_Points.begin(), m_Points.end(), [](const Point& p1, const Point& p2) {
		return p1.X < p2.X;
	});
}

void TransferFunctionEditor::Render()
{
	ImGui::Begin("Transfer Function Editor");

	ImGui::BeginChild("A", ImVec2(0.0f, 100.0f), ImGuiWindowFlags_NoScrollbar );
	ImDrawList* drawlist = ImGui::GetWindowDrawList();
	ImVec2 editorPos = ImGui::GetItemRectMin();
	ImVec2 editorSize = ImGui::GetContentRegionAvail();
	editorSize.y = std::min(150.0f, editorSize.y);

	ImVec2 mousePos = ImGui::GetMousePos();
	bool pointSelected = false;
	drawlist->AddRectFilled(editorPos, { editorPos.x + editorSize.x, editorPos.y + editorSize.y }, IM_COL32(100, 100, 100, 200));

	SortPoints();
	for (size_t i = 0; i < m_Points.size(); ++i)
	{
		auto& point = m_Points[i];
		float xPos = editorPos.x + point.X * editorSize.x;
		float yPos = editorPos.y + (1.0f - point.Y) * editorSize.y;

		if (i < m_Points.size() - 1) {
			auto& nextPoint = m_Points[i + 1];
			float nextXPos = editorPos.x + nextPoint.X * editorSize.x;
			float nextYPos = editorPos.y + (1.0f - nextPoint.Y) * editorSize.y;
			drawlist->AddLine(ImVec2(xPos, yPos), ImVec2(nextXPos, nextYPos), IM_COL32(200, 200, 200, 255), 3.0f);
		}

		ImVec2 mousePos = ImGui::GetMousePos();
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			(pow(mousePos.x - xPos, 2) + pow(mousePos.y - yPos, 2) < 100.0f)) {
			m_SelectedPoint = point.id;
			m_LastSelectedPoint = m_SelectedPoint;
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			m_SelectedPoint = -1;
		}

		if (m_SelectedPoint == point.id)
		{
			point.X = std::clamp((mousePos.x - editorPos.x) / editorSize.x, 0.0f, 1.0f);
			point.Y = 1.0f - std::clamp((mousePos.y - editorPos.y) / editorSize.y, 0.0f, 1.0f);
			drawlist->AddCircleFilled({ xPos, yPos }, 5.0f, IM_COL32(point.R * 255, point.G * 255, point.B * 255, 255));
		}
		else
		{
			drawlist->AddCircle({ xPos, yPos }, 5.0f, IM_COL32(point.R * 255, point.G * 255, point.B * 255, 255));
		}
	}
	ImGui::EndChild();

	if (ImGui::Button("Add point"))
	{
		AddPoint();
	}

	float tempColor[3] = { 0, 0, 0 };
	float* colorPtr = tempColor;
	for (auto& p : m_Points)
		if (p.id == m_LastSelectedPoint)
			colorPtr = &p.R;

	ImGui::ColorPicker3("Edit Color", colorPtr, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);

	ImGui::End();
}