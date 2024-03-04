#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec2 iTexCoord;
layout(location = 3) in mat3 iTBN;

//ayout(location = 1) in vec2 fragUv;
//ayout(location = 2) in mat3 TBN;
//
//ayout(location = 5) in vec3 camPos;
//ayout(location = 6) in vec3 lightDir;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D bindlessTextures[1024];

layout(set = 0, binding = 0) uniform PerFrameData {
	mat4 View;
	mat4 Projection;
	mat4 InvView;
	mat4 InvProjection;
} data;

layout( push_constant ) uniform constants
{
	uint colorTextureIndex;
	uint normalTextureIndex;
} push;

void main() 
{
    vec4 sampledColor = texture(bindlessTextures[push.colorTextureIndex], iTexCoord);

    vec3 normal = texture(bindlessTextures[push.normalTextureIndex], iTexCoord).xyz * 2.0 - 1.0;
    normal = normalize(iTBN * normal);

    vec3 ambient = 0.5 * sampledColor.rgb;

    vec3 cameraPosition = vec3(data.InvView[3][0], data.InvView[3][1], data.InvView[3][2]);
    //vec3 cameraPosition = vec3(data.InvView[0][3], data.InvView[1][3], data.InvView[2][3]);
    vec3 lightDir   = normalize(vec3(1, 3, -0.5));
    vec3 viewDir    = normalize(cameraPosition - iPosition);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * sampledColor.rgb;

    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.5) * spec; // assuming bright white light color

    float light = (dot(lightDir, normal) + 2) / 3;

    outColor = vec4(ambient + diffuse + specular, 1.0f);
}