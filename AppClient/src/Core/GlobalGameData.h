#pragma once

enum class GameState
{
	MainMenu = 0,
	Game,
};

class GlobalGameData
{
public:
	static GameState CurrentGameState;
	
};