#include "Application.h"

int main()
{
	Application* app = new Application();
	while (app->IsRunning())
	{
		app->Update();
	}
	delete app;
	return 0;
}