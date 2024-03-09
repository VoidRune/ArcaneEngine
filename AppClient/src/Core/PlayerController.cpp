#include "PlayerController.h"
#include "Window/Input.h"
#include <algorithm>

PlayerController::PlayerController(Arc::Window* window)
{
    Position = { 0.0f, 0.0f, -3.0f };
    WorldUp = { 0.0f, 1.0f, 0.0f };

    Offset = { -3.0f, 8.0f, -3.0f };
    Forward = glm::normalize(glm::vec3(-Offset.x, 0, -Offset.z));
    Right = glm::cross(WorldUp, Forward);
    Rotation = glm::quatLookAt(Forward, WorldUp);

    this->m_Window = window;

    m_Camera.SetProjection(70.0f, (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f));
    m_Camera.SetView(Position + Offset, Position);
}

PlayerController::~PlayerController()
{

}

bool PlayerController::SpendMana(float amount)
{
    if (m_CurrentMana >= amount)
    {
        m_CurrentMana -= amount;
        return true;
    }
    return false;
}

void PlayerController::Update(float deltaTime)
{
    m_CurrentMana = std::min(m_CurrentMana + deltaTime * m_ManaRegeneration, m_MaxMana);

    m_HasMoved = false;

    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };

    if (Arc::Input::IsKeyDown(Arc::KeyCode::W)) { velocity += Forward; m_HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::S)) { velocity -= Forward; m_HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::D)) { velocity += Right; m_HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::A)) { velocity -= Right; m_HasMoved = true; }

    velocity.y = 0.0f;
    if (velocity != glm::vec3{ 0.0f, 0.0f, 0.0f })
    {
        velocity = glm::normalize(velocity);
        glm::quat targetRotation = glm::quatLookAt(velocity, WorldUp);
        Rotation = glm::slerp(targetRotation, Rotation, exp(-deltaTime * 8.0f));
    }

    float speedBoost = 1.0f;
    if (Arc::Input::IsKeyDown(Arc::KeyCode::F)) { speedBoost = 3.0f; }
    
    Velocity = velocity * MovementSpeed * speedBoost;

    //Position += velocity * MovementSpeed * speedBoost * (float)deltaTime;
}

void PlayerController::UpdateCamera()
{
    m_Camera.SetProjection(60.0f, (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f));
    m_Camera.SetView(Position + Offset, Position);
}