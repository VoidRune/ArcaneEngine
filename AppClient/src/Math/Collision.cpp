#include "Collision.h"


bool CollisionDetection(const Circle& circle, const Rectangle& rectangle)
{
	float s = sin(rectangle.Angle);
	float c = cos(rectangle.Angle);
	// Rotate the coordinate system so that collision is Axis aligned
	glm::vec2 rotatedCircle;
	rotatedCircle.x = c * (circle.Position.x - rectangle.Position.x) - s * (circle.Position.y - rectangle.Position.y) + rectangle.Position.x;
	rotatedCircle.y = s * (circle.Position.x - rectangle.Position.x) + c * (circle.Position.y - rectangle.Position.y) + rectangle.Position.y;

	glm::vec2 nearestPoint;
	nearestPoint.x = std::max(float(rectangle.Position.x - rectangle.Width / 2.0f), std::min(rotatedCircle.x, float(rectangle.Position.x + rectangle.Width / 2.0f)));
	nearestPoint.y = std::max(float(rectangle.Position.y - rectangle.Height / 2.0f), std::min(rotatedCircle.y, float(rectangle.Position.y + rectangle.Height / 2.0f)));
	glm::vec2 rayToNearest = nearestPoint - rotatedCircle;
	float fOverlap = circle.Radius * circle.Radius - glm::length2(rayToNearest);

	//if (glm::isnan(fOverlap)) fOverlap = 0.0f;

	if (fOverlap > 0.0f)
		return true;

	return false;
}

bool CollisionDetection(const Circle& circle1, const Circle& circle2)
{
	glm::vec2 p1 = circle1.Position;
	glm::vec2 p2 = circle2.Position;
	float r1 = circle1.Radius;
	float r2 = circle2.Radius;
	if (glm::distance2(p1, p2) < (r1 + r2) * (r1 + r2))
		return true;
	return false;
}

bool CollisionResolution(Circle& circle, const Rectangle& rectangle)
{
	float s = sin(rectangle.Angle);
	float c = cos(rectangle.Angle);
	// Rotate the coordinate system so that collision is Axis aligned
	glm::vec2 rotatedCircle;
	rotatedCircle.x = c * (circle.Position.x - rectangle.Position.x) - s * (circle.Position.y - rectangle.Position.y) + rectangle.Position.x;
	rotatedCircle.y = s * (circle.Position.x - rectangle.Position.x) + c * (circle.Position.y - rectangle.Position.y) + rectangle.Position.y;

	glm::vec2 nearestPoint;
	nearestPoint.x = std::max(float(rectangle.Position.x - rectangle.Width / 2.0f), std::min(rotatedCircle.x, float(rectangle.Position.x + rectangle.Width / 2.0f)));
	nearestPoint.y = std::max(float(rectangle.Position.y - rectangle.Height / 2.0f), std::min(rotatedCircle.y, float(rectangle.Position.y + rectangle.Height / 2.0f)));
	glm::vec2 rayToNearest = nearestPoint - rotatedCircle;
	float fOverlap = circle.Radius - glm::length(rayToNearest);

	if (glm::isnan(fOverlap)) fOverlap = 0.0f;

	if (fOverlap > 0.0f && rayToNearest != glm::vec2(0.0f, 0.0f))
	{
		// Rotate back the coordinate system
		float nc = c;
		float ns = -s;
		glm::vec2 rayToNearestRotated;
		rayToNearestRotated.x = rayToNearest.x * nc - rayToNearest.y * ns;
		rayToNearestRotated.y = rayToNearest.x * ns + rayToNearest.y * nc;
		circle.Position = circle.Position - glm::normalize(rayToNearestRotated) * fOverlap;

		return true;
	}
	return false;
}

bool CollisionResolution(Circle& circle1, const Circle& circle2)
{
	glm::vec2 p1 = circle1.Position;
	glm::vec2 p2 = circle2.Position;
	float r1 = circle1.Radius;
	float r2 = circle2.Radius;
	float dist2 = glm::distance2(p1, p2);
	float rad2 = (r1 + r2) * (r1 + r2);
	if (dist2 < rad2)
	{
		float dist = glm::sqrt(dist2);
		glm::vec2 dir = glm::normalize(p1 - p2);
		circle1.Position = p2 + dir * (r1 + r2);
		return true;
	}

	return false;
}

bool Inside(const Rectangle& r1, const Rectangle& r2)
{
	glm::vec2 TL1 = r1.Position + glm::vec2{-r1.Width, r1.Height} / 2.0f;
	glm::vec2 BR1 = r1.Position + glm::vec2{ r1.Width, r1.Height} / 2.0f;
	glm::vec2 TL2 = r2.Position + glm::vec2{-r2.Width,-r2.Height} / 2.0f;
	glm::vec2 BR2 = r2.Position + glm::vec2{ r2.Width,-r2.Height} / 2.0f;

	return  TL1.x >= TL2.x &&
			TL1.y >= TL2.y &&
			BR1.x <= BR2.x &&
			BR1.y <= BR2.y;
}