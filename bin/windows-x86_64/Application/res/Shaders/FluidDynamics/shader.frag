#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D dyeImage;
layout(set = 0, binding = 1) uniform sampler2D overlayImage;

void main() 
{
	vec4 dye = texture(dyeImage, uv);
	vec4 overlay = texture(overlayImage, uv);
	outColor = dye + overlay;

	//uint mask = imageLoad(boundary, ivec2(uv * vec2(160, 90))).x;
	//outColor = vec4(mask & (1u << 0), mask & (1u << 1), mask & (1u << 2), 1);
}