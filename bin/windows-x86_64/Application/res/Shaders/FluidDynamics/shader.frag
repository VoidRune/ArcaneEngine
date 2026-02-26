#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec2 pos;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;
//layout(set = 0, binding = 0, r8ui) uniform uimage2D boundary;

void main() 
{
	vec2 uv = (pos + 1.0f) * 0.5f;
	outColor = texture(image, uv);

	//uint mask = imageLoad(boundary, ivec2(uv * vec2(160, 90))).x;
	//outColor = vec4(mask & (1u << 0), mask & (1u << 1), mask & (1u << 2), 1);
}