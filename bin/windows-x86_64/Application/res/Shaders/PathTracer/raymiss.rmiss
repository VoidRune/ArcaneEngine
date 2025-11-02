#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    vec3 dir = normalize(gl_WorldRayDirectionEXT);
    hitValue = vec3(0.2, 0.4, 0.7) * (0.5 + 0.5 * dir.y);
}