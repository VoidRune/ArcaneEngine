#include "TransferFunctionEditor.h"
#include <algorithm>

TransferFunctionEditor::TransferFunctionEditor()
{
	m_Points.push_back(Point(m_CurrentPointId++, 0.0, 0.0, 255, 0, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 0.2, 0.6, 255, 255, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 0.4, 0.1, 0, 255, 0));
	m_Points.push_back(Point(m_CurrentPointId++, 0.7, 0.2, 255, 0, 0));
	m_Points.push_back(Point(0xFFFFFF, 1.0, 0.0, 255, 0, 0));
}

TransferFunctionEditor::~TransferFunctionEditor()
{

}

void TransferFunctionEditor::AddPoint()
{
	m_Points.push_back(Point(m_CurrentPointId++, 0.5, 0.5, 255, 0, 0));
}

void TransferFunctionEditor::RemoveSelectedPoint()
{
	if (m_LastSelectedPoint == 0 || m_LastSelectedPoint == 0xFFFFFF)
		return;
	for (int32_t i = 0; i < m_Points.size(); i++)
	{
		if (m_Points[i].id == m_LastSelectedPoint)
		{
			m_Points.erase(m_Points.begin() + i);
		}
	}
}

void TransferFunctionEditor::SortPoints()
{
	std::sort(m_Points.begin(), m_Points.end(), [](const Point& p1, const Point& p2) {
		return p1.X < p2.X;
	});
}

std::vector<uint8_t> TransferFunctionEditor::GenerateTransferFunctionImage(int width)
{
	std::vector<uint8_t> imageData(width * 4);

	for (size_t i = 0; i < width; i++)
	{
		float f = i / float(width);
		Point left = m_Points[0];
		Point right = m_Points[m_Points.size() - 1];

		for (size_t j = 0; j < m_Points.size() - 1; j++)
		{
			if (m_Points[j].X >= f)
				break;
			left = m_Points[j];
			right = m_Points[j + 1];
		}
		float lRatio = (right.X - f);
		float rRatio = (f - left.X);
		float div = (right.X - left.X);

		float r = (left.R * lRatio + right.R * rRatio) / div;
		float g = (left.G * lRatio + right.G * rRatio) / div;
		float b = (left.B * lRatio + right.B * rRatio) / div;
		float density = (left.Y * lRatio + right.Y * rRatio) / div;
		imageData[i * 4 + 0] = r * 255.0f;
		imageData[i * 4 + 1] = g * 255.0f;
		imageData[i * 4 + 2] = b * 255.0f;
		imageData[i * 4 + 3] = density * 255.0f;
	}

	return imageData;

}

void TransferFunctionEditor::Render()
{
	m_HasDataChanged = false;

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
			float lastX = point.X;
			float lastY = point.Y;

			if (point.id != 0 && point.id != 0xFFFFFF)
				point.X = std::clamp((mousePos.x - editorPos.x) / editorSize.x, 0.0f, 1.0f);
			point.Y = 1.0f - std::clamp((mousePos.y - editorPos.y) / editorSize.y, 0.0f, 1.0f);
			drawlist->AddCircleFilled({ xPos, yPos }, 5.0f, IM_COL32(point.R * 255, point.G * 255, point.B * 255, 255));
			
			if (lastX != point.X || lastY != point.Y)
			{
				m_HasDataChanged = true;
			}
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
	if (ImGui::Button("Remove point"))
	{
		RemoveSelectedPoint();
	}

	float tempColor[3] = { 0, 0, 0 };
	float* colorPtr = tempColor;
	for (auto& p : m_Points)
		if (p.id == m_LastSelectedPoint)
			colorPtr = &p.R;

	tempColor[0] = colorPtr[0];
	tempColor[1] = colorPtr[1];
	tempColor[2] = colorPtr[2];
	ImGui::ColorPicker3("Edit Color", colorPtr, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
	if (tempColor[0] != colorPtr[0] || tempColor[1] != colorPtr[1] || tempColor[2] != colorPtr[2])
	{
		m_HasDataChanged = true;
	}

	ImGui::End();
}