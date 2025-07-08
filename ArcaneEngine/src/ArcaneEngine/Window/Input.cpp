#include "Input.h"
#include <vector>
#include <bitset>

namespace Arc
{
	constexpr size_t MaxKeys = static_cast<size_t>(KeyCode::KeyCount);
	static std::bitset<MaxKeys> s_PreviousInput;
	static std::bitset<MaxKeys> s_CurrentInput;
	float s_ScrollX = 0;
	float s_ScrollY = 0;
	int s_MouseX = 0;
	int s_MouseY = 0;

	void Input::Update(int mouseX, int mouseY)
	{
		s_PreviousInput = s_CurrentInput;
		s_ScrollX = 0;
		s_ScrollY = 0;
		s_MouseX = mouseX;
		s_MouseY = mouseY;
	}

	void Input::SetKey(KeyCode keyCode, bool isDown)
	{
		s_CurrentInput[(size_t)keyCode] = isDown;
	}

	void Input::SetScroll(float xscroll, float yscroll)
	{
		s_ScrollX = xscroll;
		s_ScrollY = yscroll;
	}

	bool Input::IsKeyDown(KeyCode keyCode)
	{
		return s_CurrentInput[(size_t)keyCode] == 1;
	}

	bool Input::IsKeyUp(KeyCode keyCode)
	{
		return s_CurrentInput[(size_t)keyCode] == 0;
	}

	bool Input::IsKeyPressed(KeyCode keyCode)
	{
		return s_CurrentInput[(size_t)keyCode] == 1 && s_PreviousInput[(size_t)keyCode] == 0;
	}

	bool Input::IsKeyReleased(KeyCode keyCode)
	{
		return s_CurrentInput[(size_t)keyCode] == 0 && s_PreviousInput[(size_t)keyCode] == 1;
	}

	float Input::GetScrollVertical()
	{
		return s_ScrollY;
	}

	float Input::GetScrollHorizontal()
	{
		return s_ScrollX;
	}

	float Input::GetMouseX()
	{
		return static_cast<float>(s_MouseX);
	}

	float Input::GetMouseY()
	{
		return static_cast<float>(s_MouseY);
	}

}