#pragma once
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Window/Window.h"
#include "imgui/imgui.h"

namespace Arc
{
	class ImGuiRenderer
	{
	public:
		ImGuiRenderer(Window* window, Device* device, PresentQueue* presentQueue);
		~ImGuiRenderer();

		void BeginFrame();
		void EndFrame(CommandBufferHandle cmd);
	private:
		DeviceHandle m_Device;
		DescriptorPoolHandle m_DescriptorPool;
	};
}