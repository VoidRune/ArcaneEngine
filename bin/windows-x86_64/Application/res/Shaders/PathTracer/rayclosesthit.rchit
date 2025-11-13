#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;
};
layout(set = 0, binding = 0, scalar) buffer Vertices { Vertex vertices[2]; } vertexBuffers[];
layout(set = 0, binding = 1) buffer Indices { uint indices[2]; } indexBuffers[];

hitAttributeEXT vec2 attribs;

void main()
{
    uint primID = gl_PrimitiveID;

    uint instanceIndex = gl_InstanceCustomIndexEXT;

    uint i0 = indexBuffers[instanceIndex].indices[primID * 3 + 0];
    uint i1 = indexBuffers[instanceIndex].indices[primID * 3 + 1];
    uint i2 = indexBuffers[instanceIndex].indices[primID * 3 + 2];

    vec3 p0 = vertexBuffers[instanceIndex].vertices[i0].pos;
    vec3 p1 = vertexBuffers[instanceIndex].vertices[i1].pos;
    vec3 p2 = vertexBuffers[instanceIndex].vertices[i2].pos;

    vec3 n0 = vertexBuffers[instanceIndex].vertices[i0].normal;
    vec3 n1 = vertexBuffers[instanceIndex].vertices[i1].normal;
    vec3 n2 = vertexBuffers[instanceIndex].vertices[i2].normal;

    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 localPosition = p0 * bary.x + p1 * bary.y + p2 * bary.z;
    vec3 position = gl_ObjectToWorldEXT * vec4(localPosition, 1.0);

    vec3 localNormal = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);

    payload.color = instanceIndex == 1 ? vec3(0.2, 1.0, 0.5) : vec3(0.99);
    payload.origin = position;
    payload.normal = localNormal;
    payload.hitDistance = gl_HitTEXT;
    payload.hitInfo = 1;
}