#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_buffer_reference2 : require
#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;
};
//layout(set = 0, binding = 0, scalar) buffer Vertices { Vertex vertices[2]; } vertexBuffers[];
//layout(set = 0, binding = 1) buffer Indices { uint indices[2]; } indexBuffers[];

struct MeshInfo 
{
    uvec2 vertexAddress;
    uvec2 indexAddress;
    vec4 color;
    vec4 emission;
    vec4 smoothness;
};

layout(set=0, binding=0) buffer MeshInfoBuffer { MeshInfo meshInfos[]; };

layout(buffer_reference, scalar) readonly buffer VertexBuffer { Vertex vertices[]; };
layout(buffer_reference, scalar) readonly buffer IndexBuffer { uint indices[]; };

hitAttributeEXT vec2 attribs;

void main()
{
    uint primID = gl_PrimitiveID;

    uint instanceIndex = gl_InstanceCustomIndexEXT;
    MeshInfo mi = meshInfos[instanceIndex];
    VertexBuffer vb = VertexBuffer(mi.vertexAddress);
    IndexBuffer  ib = IndexBuffer(mi.indexAddress);

    uint i0 = ib.indices[primID * 3 + 0];
    uint i1 = ib.indices[primID * 3 + 1];
    uint i2 = ib.indices[primID * 3 + 2];
    vec3 p0 = vb.vertices[i0].pos;
    vec3 p1 = vb.vertices[i1].pos;
    vec3 p2 = vb.vertices[i2].pos;
    vec3 n0 = vb.vertices[i0].normal;
    vec3 n1 = vb.vertices[i1].normal;
    vec3 n2 = vb.vertices[i2].normal;

    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 localPosition = p0 * bary.x + p1 * bary.y + p2 * bary.z;
    vec3 localNormal = n0 * bary.x + n1 * bary.y + n2 * bary.z;
    vec3 position = gl_ObjectToWorldEXT * vec4(localPosition, 1.0);
    vec3 normal = normalize(mat3(gl_WorldToObjectEXT) * localNormal);

    payload.color = mi.color.rgb;
    payload.origin = position;
    payload.normal = normal;
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
    payload.smoothness = mi.smoothness.r;
    payload.emission = mi.emission.rgb;
    payload.glossy = 0.1;
}