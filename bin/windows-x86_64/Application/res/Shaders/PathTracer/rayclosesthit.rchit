#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;
};
layout(set = 0, binding = 5, scalar) buffer Vertices {
    Vertex vertices[];
};

layout(set = 0, binding = 6) buffer Indices {
    uint indices[];
};

hitAttributeEXT vec2 attribs;

void main()
{
    uint primID = gl_PrimitiveID;

    uint i0 = indices[primID * 3 + 0];
    uint i1 = indices[primID * 3 + 1];
    uint i2 = indices[primID * 3 + 2];

    vec3 p0 = vertices[i0].pos;
    vec3 p1 = vertices[i1].pos;
    vec3 p2 = vertices[i2].pos;

    vec3 n0 = vertices[i0].normal;
    vec3 n1 = vertices[i1].normal;
    vec3 n2 = vertices[i2].normal;

    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 localPosition = p0 * bary.x + p1 * bary.y + p2 * bary.z;
    vec3 position = gl_ObjectToWorldEXT * vec4(localPosition, 1.0);

    vec3 localNormal = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);

    payload.color = vec3(1);
    payload.origin = position;
    payload.normal = localNormal;
    payload.hitInfo = 1;
}