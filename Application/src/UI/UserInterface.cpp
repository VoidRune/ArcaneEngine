#include "UserInterface.h"
#include "ArcaneEngine/Core/Log.h"

UserInterface::UserInterface()
{

}

UserInterface::~UserInterface()
{

}

void UserInterface::SetupDockspace()
{
	ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	dockspace_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	dockspace_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Dockspace", nullptr, dockspace_flags);
	ImGui::PopStyleVar(3);

	ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void UserInterface::EndDockspace()
{
	ImGui::End();
}

void UserInterface::RenderMenuBar(const std::vector<std::string>& files, std::function<void(std::string)>&& onSelect)
{
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if(ImGui::BeginMenu("Open"))
			{
				for (auto& f : files)
				{
					if (ImGui::MenuItem(f.c_str()))
					{
						onSelect(f);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void UserInterface::RenderCanvas(ImTextureID& displayImage, std::function<void(float, float)>&& func)
{
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
	ImGui::SetNextWindowClass(&window_class);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoTitleBar);
	ImGui::PopStyleVar();
	ImVec2 canvasRegion = ImGui::GetContentRegionAvail();

	func(canvasRegion.x, canvasRegion.y);

	ImGui::Image(displayImage, canvasRegion);
	ImGui::End();
}