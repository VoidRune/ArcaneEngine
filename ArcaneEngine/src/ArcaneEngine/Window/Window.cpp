#include "Window.h"
#include "Input.h"

#define NOMINMAX
#define NOGDI
#include <GLFW/glfw3.h>

#include <iostream>

namespace Arc
{
	struct InputData
	{
		void (*SetKey)(KeyCode keyCode, char action);
		void (*SetScroll)(float xscroll, float yscroll);
	} inputFunctions;

	Window::Window(const WindowDescription& desc)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
		if (!desc.Titlebar)
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		int width = desc.Width;
		int height = desc.Height;

		m_IsFullscreen = desc.Fullscreen;
		m_WindowedPos[0] = static_cast<int>((mode->width - desc.Width) * 0.5);
		m_WindowedPos[1] = static_cast<int>((mode->height - desc.Height) * 0.5);
		m_WindowedSize[0] = desc.Width;
		m_WindowedSize[1] = desc.Height;

		if (desc.Fullscreen)
		{
			width = mode->width;
			height = mode->height;
		}
		m_Window = glfwCreateWindow(width, height, desc.Title.c_str(), desc.Fullscreen ? monitor : nullptr, nullptr);
				
		Input::Init();

		inputFunctions.SetKey = Input::SetKey;
		inputFunctions.SetScroll = Input::SetScroll;

		glfwSetWindowUserPointer((GLFWwindow*)m_Window, &inputFunctions);

		glfwSetKeyCallback((GLFWwindow*)m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				InputData& inputFunc = *(InputData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					inputFunc.SetKey((KeyCode)key, 1);
					break;
				}
				case GLFW_RELEASE:
				{
					inputFunc.SetKey((KeyCode)key, 0);
					break;
				}
				case GLFW_REPEAT:
				{

					break;
				}
				}
			});

		glfwSetMouseButtonCallback((GLFWwindow*)m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				InputData& inputFunc = *(InputData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					inputFunc.SetKey((KeyCode)button, 1);
					break;
				}
				case GLFW_RELEASE:
				{
					inputFunc.SetKey((KeyCode)button, 0);
					break;
				}
				}
			});

		glfwSetScrollCallback((GLFWwindow*)m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				InputData& inputFunc = *(InputData*)glfwGetWindowUserPointer(window);

				inputFunc.SetScroll((float)xOffset, (float)yOffset);
			});
	}

	Window::~Window()
	{
		glfwDestroyWindow((GLFWwindow*)m_Window);
		glfwTerminate();
	}

	void Window::PollEvents()
	{
		double xpos, ypos;
		glfwGetCursorPos((GLFWwindow*)m_Window, &xpos, &ypos);
		Input::Update((int)xpos, (int)ypos);
		glfwPollEvents();
	}

	void Window::WaitEvents()
	{
		glfwWaitEvents();
	}

	void Window::SetClosed(bool close)
	{
		glfwSetWindowShouldClose((GLFWwindow*)m_Window, close);
	}

	bool Window::IsClosed()
	{
		return glfwWindowShouldClose((GLFWwindow*)m_Window);
	}

	void Window::SetFullscreen(bool fullscreen)
	{
		if (m_IsFullscreen == fullscreen)
			return;

		if (fullscreen)
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			glfwGetWindowPos((GLFWwindow*)m_Window, &m_WindowedPos[0], &m_WindowedPos[1]);
			glfwGetWindowSize((GLFWwindow*)m_Window, &m_WindowedSize[0], &m_WindowedSize[1]);

			glfwSetWindowMonitor((GLFWwindow*)m_Window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
		}
		else
		{
			glfwSetWindowMonitor((GLFWwindow*)m_Window, nullptr, m_WindowedPos[0], m_WindowedPos[1], m_WindowedSize[0], m_WindowedSize[1], GLFW_DONT_CARE);
		}

		m_IsFullscreen = fullscreen;
	}

	bool Window::IsFullscreen()
	{
		return m_IsFullscreen;
	}

	int Window::Width()
	{
		int windowWidth;
		int windowHeight;
		glfwGetWindowSize((GLFWwindow*)m_Window, &windowWidth, &windowHeight);
		return windowWidth;
	}

	int Window::Height()
	{
		int windowWidth;
		int windowHeight;
		glfwGetWindowSize((GLFWwindow*)m_Window, &windowWidth, &windowHeight);
		return windowHeight;
	}

	std::vector<const char*> Window::GetInstanceExtensions()
	{
		uint32_t extensionCount = 0;
		const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		std::vector<const char*> instanceExtensions(extensions, extensions + extensionCount);
		return instanceExtensions;
	}
}