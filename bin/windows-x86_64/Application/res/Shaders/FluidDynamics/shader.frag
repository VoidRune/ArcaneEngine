#version 450

layout (location = 0) in vec2 pos;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;
//layout(set = 0, binding = 1) uniform sampler2D radiancaCascade0;

void main() 
{
	vec2 uv = (pos + 1.0f) * 0.5f;
	outColor = texture(image, uv);
}