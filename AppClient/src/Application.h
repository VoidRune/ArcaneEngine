#pragma once
#include <memory>
#include "Core/Game.h"

class Application
{
public:
	Application();
	~Application();

	void Run();

private:
	std::unique_ptr<Arc::Window> m_Window;
	std::unique_ptr<Arc::Device> m_Device;
	std::unique_ptr<Arc::PresentQueue> m_PresentQueue;
	std::unique_ptr<AssetCache> m_AssetCache;
	std::unique_ptr<Game> m_Game;
};