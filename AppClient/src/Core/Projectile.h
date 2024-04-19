#pragma once
#include "Math/Math.h"
	
struct Projectile
{
	glm::vec3 Position;
	glm::vec3 Velocity;
	float Angle;
	float LifeTime;
	bool Deadly;
};