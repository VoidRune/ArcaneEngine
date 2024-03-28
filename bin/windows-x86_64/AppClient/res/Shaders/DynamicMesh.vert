#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec3 iTangent;
layout(location = 3) in vec2 iTexCoord;
layout(location = 4) in vec4 iBoneIndices;
layout(location = 5) in vec4 iBoneWeights;

layout(location = 0) out vec3 oPosition;
layout(location = 1) out vec4 oLightPosition;
layout(location = 2) out vec3 oNormal;
layout(location = 3) out vec2 oTexCoord;
layout(location = 4) out mat3 oTBN;

//layout(location = 5) out vec3 camPos;
//layout(location = 6) out vec3 lightDir;

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
	uint colorTextureIndex;
	uint normalTextureIndex;
	uint boneMatricesStart;
} push;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() 
{
    mat4 skinMat =
        iBoneWeights.x * boneMatrices[push.boneMatricesStart + int(iBoneIndices.x)] +
        iBoneWeights.y * boneMatrices[push.boneMatricesStart + int(iBoneIndices.y)] +
        iBoneWeights.z * boneMatrices[push.boneMatricesStart + int(iBoneIndices.z)] +
        iBoneWeights.w * boneMatrices[push.boneMatricesStart + int(iBoneIndices.w)];
	mat4 transform = transformMatrices[gl_InstanceIndex] * skinMat;

	vec3 T = normalize(vec3(transform * vec4(iTangent, 0.0)));
	vec3 N = normalize(vec3(transform * vec4(iNormal, 0.0)));
	vec3 B = cross(N, T);

	vec4 pos = transform * vec4(iPosition, 1.0);
	oPosition = pos.xyz;
	oLightPosition = biasMat * ShadowViewProj * pos;
	oNormal = iNormal;
	oTexCoord = iTexCoord;
	oTBN = mat3(T, B, N);
	gl_Position = Projection * View * pos;
}