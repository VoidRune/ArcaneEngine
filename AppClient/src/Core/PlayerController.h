#pragma once
#include "Window/Window.h"
#include "Camera.h"

class PlayerController
{
public:
	PlayerController(Arc::Window* window);
	~PlayerController();

	void UpdateVelocity(double deltaTime);
	void UpdateCamera();

	void SetPosition(glm::vec3 position) { Position = position; };
	void SetMovementSpeed(float movementSpeed) { MovementSpeed = movementSpeed; };

	Camera& GetCamera() { return m_Camera; }
	glm::vec3& GetPosition() { return Position; }
	glm::vec3 GetVelocity() { return Velocity; }
	bool HasMoved() { return m_HasMoved; }

private:
	Camera m_Camera;

	float MovementSpeed = 2.0f;
	bool m_HasMoved;

	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 Offset;

	glm::vec3 Forward;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	Arc::Window* m_Window;
};