#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) in vec3 iNormal;
//ayout(location = 1) in vec2 fragUv;
//ayout(location = 2) in mat3 TBN;
//
//ayout(location = 5) in vec3 camPos;
//ayout(location = 6) in vec3 lightDir;

layout(location = 0) out vec4 outColor;

//layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
//layout(set = 1, binding = 1) uniform sampler2D normalTexture;
//layout(set = 1, binding = 2) uniform sampler2D metalicRoughnessTexture;

void main() 
{
    outColor = vec4(iNormal, 1.0);
}