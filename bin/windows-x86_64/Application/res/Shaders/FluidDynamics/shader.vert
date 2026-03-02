#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec2 uv;

vec2 positions[4] = vec2[](
    vec2(-1,-1),
    vec2( 1,-1),
    vec2( 1, 1),
    vec2(-1, 1)
);

void main()
{
    vec2 pos = positions[gl_VertexIndex];
    uv = (pos + 1.0f) * 0.5f;
    gl_Position = vec4(pos, 0.0, 1.0);
}