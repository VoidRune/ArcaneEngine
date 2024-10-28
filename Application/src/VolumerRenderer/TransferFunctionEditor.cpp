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

std::vector<uint32_t> TransferFunctionEditor::GenerateTransferFunctionImage(int width)
{
	// Create the 1D image array
	std::vector<uint32_t> imageData(width); // RGBA for each pixel

	// Linear interpolation between points
	for (int i = 0; i < width; ++i) {
		float t = static_cast<float>(i) / (width - 1); // Normalized [0, 1] across image width

		// Find the two closest control points
		auto upper = std::upper_bound(m_Points.begin(), m_Points.end(), t, [](float t, const Point& point) {
			return t < point.X;
		});

		if (upper == m_Points.begin()) {
			// If t is less than the first point, use the first point color
			const auto& p = m_Points.front();
			imageData[i] = (int)(p.R * 255) << 24 | (int)(p.G * 255) << 16 | (int)(p.R * 255) << 8 | (int)(p.Y * 255);
		}
		else if (upper == m_Points.end()) {
			// If t is beyond the last point, use the last point color
			const auto& p = m_Points.back();
			imageData[i] = (int)(p.R * 255) << 24 | (int)(p.G * 255) << 16 | (int)(p.R * 255) << 8 | (int)(p.Y * 255);
		}
		else {
			// Interpolate between the two nearest points
			const auto& p1 = *(upper - 1);
			const auto& p2 = *upper;
			float localT = (t - p1.X) / (p2.X - p1.X); // Interpolation factor between p1 and p2

			// Interpolated color and opacity
			int r = p1.R + localT * (p2.R - p1.R);
			int g = p1.G + localT * (p2.G - p1.G);
			int b = p1.B + localT * (p2.B - p1.B);
			int a = p1.Y + localT * (p2.Y - p1.Y);

			// Store interpolated color in image data
			imageData[i] = r << 24 | g << 16 | b << 8 | a;
		}
	}

	return imageData; // The generated 1D RGBA transfer function image

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
			if (point.id != 0 && point.id != 0xFFFFFF)
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
	if (ImGui::Button("Remove point"))
	{
		RemoveSelectedPoint();
	}

	float tempColor[3] = { 0, 0, 0 };
	float* colorPtr = tempColor;
	for (auto& p : m_Points)
		if (p.id == m_LastSelectedPoint)
			colorPtr = &p.R;

	ImGui::ColorPicker3("Edit Color", colorPtr, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);

	ImGui::End();
}