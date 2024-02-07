#pragma once
#include "KeyCodes.h"

namespace Arc
{
	class Input
	{
	public:
		static bool IsKeyDown(KeyCode keyCode);
		static bool IsKeyUp(KeyCode keyCode);
		static bool IsKeyPressed(KeyCode keyCode);
		static bool IsKeyReleased(KeyCode keyCode);
		static float GetScrollVertical();
		static float GetScrollHorizontal();
		static float GetMouseX();
		static float GetMouseY();

	private:
		static void Init();
		static void Update(int mouseX, int mouseY);

		static void SetKey(KeyCode keyCode, char action);
		static void SetScroll(float xscroll, float yscroll);

		friend class Window;
	};

}