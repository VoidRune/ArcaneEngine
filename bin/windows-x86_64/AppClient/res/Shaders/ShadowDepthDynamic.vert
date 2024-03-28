#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec3 iTangent;
layout(location = 3) in vec2 iTexCoord;
layout(location = 4) in vec4 iBoneIndices;
layout(location = 5) in vec4 iBoneWeights;

layout(set = 0, binding = 0) uniform PerFrameData {
	mat4 View;
	mat4 Projection;
	mat4 InvView;
	mat4 InvProjection;
	mat4 OrthoProjection;
	mat4 ShadowViewProj;
};

layout(std140,set = 1, binding = 0) readonly buffer TransformMatrices{
    mat4 transformMatrices[];
};

layout(std140,set = 1, binding = 1) readonly buffer BoneMatrices{
    mat4 boneMatrices[];
};

layout( push_constant ) uniform constants
{
	uint boneMatricesStart;
} push;

void main() 
{
    mat4 skinMat =
        iBoneWeights.x * boneMatrices[push.boneMatricesStart + int(iBoneIndices.x)] +
        iBoneWeights.y * boneMatrices[push.boneMatricesStart + int(iBoneIndices.y)] +
        iBoneWeights.z * boneMatrices[push.boneMatricesStart + int(iBoneIndices.z)] +
        iBoneWeights.w * boneMatrices[push.boneMatricesStart + int(iBoneIndices.w)];
	mat4 transform = transformMatrices[gl_InstanceIndex] * skinMat;

	vec4 pos = transform * vec4(iPosition, 1.0);
	gl_Position = ShadowViewProj * pos;
}