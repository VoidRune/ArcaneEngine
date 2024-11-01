#include "CameraFP.h"
#include "ArcaneEngine/Window/Input.h"
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>

CameraFP::CameraFP(Arc::Window* window)
{
    Position = { 0.0f, 0.0f, -3.0f };
    Forward = glm::vec3(0.0f, 0.0f, 1.0f);
    WorldUp = { 0.0f, 1.0f, 0.0f };

    this->m_Window = window;
    UpdateMatrices(1);
}

CameraFP::~CameraFP()
{

}

void CameraFP::Update(double deltaTime, float aspectRatio)
{
    HasMoved = false;
    double mouseX = Arc::Input::GetMouseX();
    double mouseY = Arc::Input::GetMouseY();

    if (Arc::Input::IsKeyDown(Arc::KeyCode::MouseRight))
    {
        Yaw -= (mouseX - LastMouseX) * Sensitivity;
        Pitch -= (mouseY - LastMouseY) * Sensitivity;

        Yaw = fmod(Yaw, 360.0f);
        Pitch = std::clamp(Pitch, -89.0f, 89.0f);
        if (LastMouseX != mouseX || LastMouseY != mouseY)
            HasMoved = true;
    }
    LastMouseX = mouseX;
    LastMouseY = mouseY;

    Forward = glm::normalize(glm::vec3{
        cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
        sin(glm::radians(Pitch)),
        sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))
        });
    Right = glm::cross(-Forward, WorldUp);
    Right = glm::normalize(Right);
    Up = glm::cross(Forward, Right);

    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };

    if (Arc::Input::IsKeyDown(Arc::KeyCode::W)) { velocity += Forward; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::S)) { velocity -= Forward; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::D)) { velocity += Right; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::A)) { velocity -= Right; HasMoved = true; }

    velocity.y = 0.0f;
    if (velocity != glm::vec3{ 0.0f, 0.0f, 0.0f })
        velocity = glm::normalize(velocity);

    if (Arc::Input::IsKeyDown(Arc::KeyCode::Space)) { velocity.y++; HasMoved = true; }
    if (Arc::Input::IsKeyDown(Arc::KeyCode::LeftShift)) { velocity.y--; HasMoved = true; }

    Velocity = velocity;

    float speedBoost = 1.0f;
    if (Arc::Input::IsKeyDown(Arc::KeyCode::F)) { speedBoost = 10.0f; }

    Position += velocity * MovementSpeed * speedBoost * (float)deltaTime;
    UpdateMatrices(aspectRatio);
}

void CameraFP::UpdateMatrices(float aspectRatio)
{
    Projection = glm::perspective(glm::radians(Fov), aspectRatio, NearPlane, FarPlane);
    Projection[1][1] *= -1;
    View = glm::lookAt(Position, Position + Forward, WorldUp);
    InverseView = glm::inverse(View);
    InverseProjection = glm::inverse(Projection);
}