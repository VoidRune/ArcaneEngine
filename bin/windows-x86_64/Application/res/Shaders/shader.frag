#version 450

layout(location = 0) in vec2 pos;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;

void main() 
{
	vec2 uv = (pos + 1) * 0.5f;
	outColor = vec4(texture(image, uv));
}