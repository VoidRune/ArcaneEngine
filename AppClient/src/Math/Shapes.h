#pragma once
#include "Math.h"

struct Point
{
	glm::vec2 Position;
};

struct Circle
{
	glm::vec2 Position;
	float Radius;
};

struct Rectangle
{
	glm::vec2 Position;
	float Width, Height;
	float Angle;
};

struct AABB
{
	glm::vec2 TL;
	glm::vec2 BR;
};