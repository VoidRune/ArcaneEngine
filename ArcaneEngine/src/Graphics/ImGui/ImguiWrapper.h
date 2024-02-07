#pragma once
#include "Graphics/Device.h"
#include "Graphics/PresentQueue.h"
#include "../dependencies/imgui-master/imgui.h"
#include "../dependencies/imgui-master/imgui_impl_glfw.h"
#include "../dependencies/imgui-master/imgui_impl_vulkan.h"

namespace Arc
{
	void ImGuiInit(void* window, Device* _core, PresentQueue* _presentQueue);
	void ImGuiShutdown();

	void ImGuiBeginFrame();
	void ImGuiEndFrame(VkCommandBuffer cmd);
}