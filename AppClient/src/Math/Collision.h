#pragma once
#include "Shapes.h"

bool CollisionDetection(const Circle& circle, const Rectangle& rectangle);
bool CollisionDetection(const Circle& circle1, const Circle& circle2);

bool CollisionResolution(Circle& circle, const Rectangle& rectangle);
bool CollisionResolution(Circle& circle1, const Circle& circle2);