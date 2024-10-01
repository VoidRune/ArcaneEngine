#pragma once

#include <string>
#include <vector>

namespace Arc
{
	struct WindowDescription
	{
	public:
		std::string Title = "Application";
		uint32_t Width = 1280;
		uint32_t Height = 720;
		bool Fullscreen = false;
	};

	class Window
	{
	public:
		Window(const WindowDescription& desc);
		~Window();

		void PollEvents();
		void WaitEvents();

		void SetClosed(bool close);
		bool IsClosed();
		void SetFullscreen(bool fullscreen);
		bool IsFullscreen();

		int Width();
		int Height();

		void* GetHandle() { return m_Window; }
		std::vector<const char*> GetInstanceExtensions();

	private:
		void* m_Window;
		bool m_IsFullscreen;

		int m_WindowedPos[2];
		int m_WindowedSize[2];
	};
}