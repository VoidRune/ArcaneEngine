#include "Input.h"
#include <vector>

namespace Arc
{
	std::vector<char> s_PreviousInput;
	std::vector<char> s_CurrentInput;
	float s_ScrollX;
	float s_ScrollY;
	int s_MouseX;
	int s_MouseY;

	void Input::Init()
	{
		s_PreviousInput.resize((size_t)KeyCode::KeyCount);
		s_CurrentInput.resize((size_t)KeyCode::KeyCount);
		s_ScrollX = 0;
		s_ScrollY = 0;
		s_MouseX = 0;
		s_MouseY = 0;
	}

	void Input::Update(int mouseX, int mouseY)
	{
		memcpy(s_PreviousInput.data(), s_CurrentInput.data(), s_CurrentInput.size() * sizeof(char));
		s_ScrollX = 0;
		s_ScrollY = 0;
		s_MouseX = mouseX;
		s_MouseY = mouseY;
	}

	void Input::SetKey(KeyCode keyCode, char action)
	{
		s_CurrentInput[(size_t)keyCode] = action;
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