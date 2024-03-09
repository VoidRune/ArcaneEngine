#pragma once
#include "Window/Window.h"
#include "Math/Math.h"

class Camera
{
public:
	Camera();
	~Camera();

	void SetProjection(float fov, float aspectRatio);
	void SetView(glm::vec3 eye, glm::vec3 lookAt);

	glm::vec3 ProjectRay(float x, float y);

	float Theta = 0.0f;
	float Phi = 0.0f;
	float Pitch = 0.0f;
	float Yaw = 90.0f;
	float Roll = 0.0f;
	float Fov = 60.0f;
	float NearPlane = 0.1f;
	float FarPlane = 1000.0f;

	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 InverseProjection;
	glm::mat4 InverseView;

	glm::vec3 Position;
	glm::vec3 Forward;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
};