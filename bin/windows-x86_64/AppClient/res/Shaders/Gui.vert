#version 450 core
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) in vec2 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec4 iColor;

layout(location = 0) out vec2 oTexCoord;
layout(location = 1) out vec4 oColor;

layout(set = 0, binding = 0) uniform PerFrameData {
	mat4 View;
	mat4 Projection;
	mat4 InvView;
	mat4 InvProjection;
    mat4 OrthoProjection;
};

void main() 
{
	oTexCoord = iTexCoord;
	oColor = iColor;
	gl_Position = OrthoProjection * vec4(iPosition, 0.0, 1.0);
}