#include "CameraFPS.h"
#include "Window/Input.h"
#include <algorithm>

CameraFPS::CameraFPS(Arc::Window* window)
{
    Position = { 0.0f, 0.0f, -3.0f };
    Forward = glm::vec3(0.0f, 0.0f, 1.0f);
    Right = { 1.0f, 0.0f, 0.0f };
    WorldUp = { 0.0f, 1.0f, 0.0f };

    this->m_Window = window;

    View = glm::lookAt(Position, Position + Forward, WorldUp);
    Projection = glm::perspective(glm::radians(Fov), (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f), NearPlane, FarPlane);
    Projection[1][1] *= -1;
    InverseView = glm::inverse(View);
    InverseProjection = glm::inverse(Projection);
}

CameraFPS::~CameraFPS()
{

}

void CameraFPS::Update(double deltaTime)
{
    HasMoved = false;

    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };

    if (Arc::Input::IsKeyDown(Arc::KeyCode::W)) { velocity += Forward; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::S)) { velocity -= Forward; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::D)) { velocity += Right; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::A)) { velocity -= Right; HasMoved = true; }

    velocity.y = 0.0f;
    if (velocity != glm::vec3{ 0.0f, 0.0f, 0.0f })
        velocity = glm::normalize(velocity);

    Velocity = velocity;

    float speedBoost = 1.0f;
    if (Arc::Input::IsKeyDown(Arc::KeyCode::F)) { speedBoost = 10.0f; }

    Position += velocity * MovementSpeed * speedBoost * (float)deltaTime;

    glm::vec3 offset = glm::vec3(0, 5, -2);

    Projection = glm::perspective(glm::radians(Fov), (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f), NearPlane, FarPlane);
    Projection[1][1] *= -1;
    View = glm::lookAt(Position + offset, Position, WorldUp);
    InverseView = glm::inverse(View);
    InverseProjection = glm::inverse(Projection);
}