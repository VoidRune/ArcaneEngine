#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec3 iTangent;
layout(location = 3) in vec2 iTexCoord;
layout(location = 4) in vec4 iBoneIndices;
layout(location = 5) in vec4 iBoneWeights;

layout(location = 0) out vec3 oPosition;
layout(location = 1) out vec3 oNormal;
layout(location = 2) out vec2 oTexCoord;
layout(location = 3) out mat3 oTBN;

//layout(location = 5) out vec3 camPos;
//layout(location = 6) out vec3 lightDir;

layout(set = 0, binding = 0) uniform PerFrameData {
	mat4 View;
	mat4 Projection;
	mat4 InvView;
	mat4 InvProjection;
} data;

layout(std140,set = 0, binding = 1) readonly buffer TransformMatrices{
    mat4 transformMatrices[];
};

layout(std140,set = 0, binding = 2) readonly buffer BoneMatrices{
    mat4 boneMatrices[];
};

void main() 
{
    mat4 skinMat =
        iBoneWeights.x * boneMatrices[int(iBoneIndices.x)] +
        iBoneWeights.y * boneMatrices[int(iBoneIndices.y)] +
        iBoneWeights.z * boneMatrices[int(iBoneIndices.z)] +
        iBoneWeights.w * boneMatrices[int(iBoneIndices.w)];
	mat4 transform = transformMatrices[gl_InstanceIndex] * skinMat;

	vec3 T = normalize(vec3(transform * vec4(iTangent, 0.0)));
	vec3 N = normalize(vec3(transform * vec4(iNormal, 0.0)));
	vec3 B = cross(N, T);

	vec4 pos = transform * vec4(iPosition, 1.0);
	oPosition = pos.xyz;
	oNormal = iNormal;
	oTexCoord = iTexCoord;
	oTBN = mat3(T, B, N);
	gl_Position = data.Projection * data.View * pos;
}