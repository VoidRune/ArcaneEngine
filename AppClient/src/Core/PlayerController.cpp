#include "PlayerController.h"
#include "Window/Input.h"
#include <algorithm>

PlayerController::PlayerController(Arc::Window* window)
{
    Position = { 0.0f, 0.0f, -3.0f };
    Forward = { 0.0f, 0.0f, 1.0f };
    Right = { 1.0f, 0.0f, 0.0f };
    WorldUp = { 0.0f, 1.0f, 0.0f };

    Offset = { 0.0f, 7.0f, -4.0f };
    this->m_Window = window;

    m_Camera.SetProjection(70.0f, (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f));
    m_Camera.SetView(Position + Offset, Position);
}

PlayerController::~PlayerController()
{

}

void PlayerController::UpdateVelocity(double deltaTime)
{
    m_HasMoved = false;

    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };

    if (Arc::Input::IsKeyDown(Arc::KeyCode::W)) { velocity += Forward; m_HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::S)) { velocity -= Forward; m_HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::D)) { velocity += Right; m_HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::A)) { velocity -= Right; m_HasMoved = true; }

    velocity.y = 0.0f;
    if (velocity != glm::vec3{ 0.0f, 0.0f, 0.0f })
        velocity = glm::normalize(velocity);

    float speedBoost = 1.0f;
    if (Arc::Input::IsKeyDown(Arc::KeyCode::F)) { speedBoost = 3.0f; }
    
    Velocity = velocity * MovementSpeed * speedBoost;

    //Position += velocity * MovementSpeed * speedBoost * (float)deltaTime;
}

void PlayerController::UpdateCamera()
{
    m_Camera.SetProjection(70.0f, (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f));
    m_Camera.SetView(Position + Offset, Position);
}