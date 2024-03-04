#include "Camera.h"
#include "Window/Input.h"
#include <algorithm>

Camera::Camera()
{
    Position = { 0.0f, 0.0f, -3.0f };
    Forward = glm::vec3(0.0f, 0.0f, 1.0f);
    Right = { 1.0f, 0.0f, 0.0f };
    WorldUp = { 0.0f, 1.0f, 0.0f };
}

Camera::~Camera()
{

}

void Camera::SetProjection(float fov, float aspectRatio)
{
    Fov = fov;
    Projection = glm::perspective(glm::radians(Fov), aspectRatio, NearPlane, FarPlane);
    Projection[1][1] *= -1;
    InverseProjection = glm::inverse(Projection);
}

void Camera::SetView(glm::vec3 eye, glm::vec3 lookAt)
{
    Position = eye;
    View = glm::lookAt(Position, lookAt, WorldUp);
    InverseView = glm::inverse(View);
}

glm::vec3 Camera::ProjectRay(float x, float y)
{
    //glm::vec2 coord = { (float)x / m_Window->Width(), (float)y / m_Window->Height() };
    glm::vec2 coord = glm::vec2(x, y) * 2.0f - 1.0f;
    glm::vec4 target = InverseProjection * glm::vec4(coord.x, coord.y, 1, 1);
    glm::vec3 rayDirection = glm::vec3(InverseView * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));
    
    return rayDirection;
}