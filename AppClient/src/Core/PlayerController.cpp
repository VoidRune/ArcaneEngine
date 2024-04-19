#include "PlayerController.h"
#include "Window/Input.h"
#include "Math/Collision.h"
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

    AssetCache::LoadGltf(&m_FrozenLanceMesh, "res/Meshes/spear.glb");
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

void PlayerController::UpdateSpells(float deltaTime, std::vector<Projectile>& projectiles, RenderFrameData& frameData)
{
    bool casting = false;
    bool release = false;
    m_CastTime += deltaTime;

    if (Arc::Input::IsKeyDown(Arc::KeyCode::MouseLeft))
        casting = true;
    if (Arc::Input::IsKeyReleased(Arc::KeyCode::MouseLeft))
    {
        release = true;
        casting = false;
    }
    //release = true;
    m_IsCasting = casting;

    if (casting || release)
    {
        glm::vec3 point;
        RayPlaneCollision(Ray{ m_Camera.Position, m_Camera.ProjectRay(Arc::Input::GetMouseX() / (float)m_Window->Width(), Arc::Input::GetMouseY() / (float)m_Window->Height()) },
            Plane{ {0, 0, 0}, {0, 1, 0} }, point);

        glm::vec3 dir = glm::normalize(point - Position);
        glm::vec3 leftDir = glm::cross(dir, WorldUp) * 0.6f;
        glm::vec3 rightPoint = Position - leftDir;
        glm::vec3 leftPoint = Position + leftDir;
        glm::vec3 dir1 = glm::normalize(point - rightPoint);
        glm::vec3 dir2 = glm::normalize(point - leftPoint);

        float angle = glm::acos(dir.z) * glm::sign(dir.x);
        float angle1 = glm::acos(dir1.z) * glm::sign(dir1.x);
        float angle2 = glm::acos(dir2.z) * glm::sign(dir2.x);

        float castTime = m_CastTime;
        if (castTime > 1.0f) castTime = 1.0f;

        if (casting)
        {
            std::vector<glm::mat4> transforms;
            transforms.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), Position + dir * (0.4f - castTime * 0.2f) + glm::vec3(0, 1, 0)), angle, glm::vec3(0, 1, 0)));
            transforms.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), rightPoint + dir1 * (0.4f - castTime * 0.2f) + glm::vec3(0, 1, 0)), angle1, glm::vec3(0, 1, 0)));
            transforms.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), leftPoint + dir2 * (0.4f - castTime * 0.2f) + glm::vec3(0, 1, 0)), angle2, glm::vec3(0, 1, 0)));

            frameData.AddStaticDrawData(Model(m_FrozenLanceMesh, Texture{ Arc::GpuImage(), 0 }, Texture{ Arc::GpuImage(), 1 }), transforms);

        }
        else
        {
            Projectile p;
            p.Position = Position + dir * (0.4f - castTime * 0.2f) + glm::vec3(0, 1, 0);
            p.Velocity = dir * 24.0f;
            p.Angle = angle;
            p.LifeTime = glm::clamp(castTime, 0.1f, 0.6f);
            p.Deadly = false;
            projectiles.push_back(p);
            p.Position = rightPoint + dir1 * (0.4f - castTime * 0.2f) + glm::vec3(0, 1, 0);
            p.Velocity = dir1 * 24.0f;
            p.Angle = angle1;
            projectiles.push_back(p);
            p.Position = leftPoint + dir2 * (0.4f - castTime * 0.2f) + glm::vec3(0, 1, 0);
            p.Velocity = dir2 * 24.0f;
            p.Angle = angle2;
            projectiles.push_back(p);
        }
    }
    else
    {
        m_CastTime = 0.0f;
    }
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
    if (m_IsCasting) speedBoost = 0.35f;
    if (Arc::Input::IsKeyDown(Arc::KeyCode::F)) { speedBoost = 3.0f; }
    
    Velocity = velocity * MovementSpeed * speedBoost;

    //Position += velocity * MovementSpeed * speedBoost * (float)deltaTime;
}

void PlayerController::UpdateCamera()
{
    m_Camera.SetProjection(60.0f, (float)m_Window->Width() / glm::max((float)m_Window->Height(), 1.0f));
    m_Camera.SetView(Position + Offset, Position);
}