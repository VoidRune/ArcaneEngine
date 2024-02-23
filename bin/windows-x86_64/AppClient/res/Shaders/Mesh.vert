#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec2 iTexCoord;

layout(location = 0) out vec3 oNormal;
//layout(location = 1) out vec2 fragUv;
//layout(location = 2) out mat3 TBN;
//
//layout(location = 5) out vec3 camPos;
//layout(location = 6) out vec3 lightDir;

layout(set = 0, binding = 0) uniform PerFrameData {
	mat4 View;
	mat4 Projection;
} data;

//all object matrices
//layout(std140,set = 0, binding = 1) readonly buffer MeshMatrices{
//    mat4 matrix[];
//} meshMatrices;

layout( push_constant ) uniform constants
{
	mat4 transform;
} PushConstants;

void main() 
{
	oNormal = iNormal;
	gl_Position = data.Projection * data.View * PushConstants.transform * vec4(iPosition, 1.0);
}