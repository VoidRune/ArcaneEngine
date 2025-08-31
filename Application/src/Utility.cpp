#include "Utility.h"

glm::vec3 HsvToRgb(float h, float s, float v)
{
	h = std::fmod(h, 1.0f);
	float c = v * s;
	float x = c * (1 - std::fabs(fmod(h * 6.0f, 2.0f) - 1));
	float m = v - c;

	glm::vec3 rgb;

	if (h < 1.0f / 6.0f)       rgb = glm::vec3(c, x, 0);
	else if (h < 2.0f / 6.0f)  rgb = glm::vec3(x, c, 0);
	else if (h < 3.0f / 6.0f)  rgb = glm::vec3(0, c, x);
	else if (h < 4.0f / 6.0f)  rgb = glm::vec3(0, x, c);
	else if (h < 5.0f / 6.0f)  rgb = glm::vec3(x, 0, c);
	else                      rgb = glm::vec3(c, 0, x);

	return rgb + glm::vec3(m);
}