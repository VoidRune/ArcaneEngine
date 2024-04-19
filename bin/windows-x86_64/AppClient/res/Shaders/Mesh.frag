#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iLightPosition;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec2 iTexCoord;
layout(location = 4) in mat3 iTBN;

//ayout(location = 1) in vec2 fragUv;
//ayout(location = 2) in mat3 TBN;
//
//ayout(location = 5) in vec3 camPos;
//ayout(location = 6) in vec3 lightDir;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform PerFrameData {
	mat4 View;
	mat4 Projection;
	mat4 InvView;
	mat4 InvProjection;
	mat4 OrthoProjection;
	mat4 ShadowViewProj;
	vec3 CameraPosition;
	vec3 SunShadowDirection;
};

layout(set = 1, binding = 2) uniform sampler2D Shadowmap;

layout(set = 2, binding = 0) uniform sampler2D bindlessTextures[1024];

layout( push_constant ) uniform constants
{
	uint colorTextureIndex;
	uint normalTextureIndex;
	uint boneMatricesStart;
};

float CalculateShadow()
{
    vec3 projCoords = iLightPosition.xyz / iLightPosition.w;

    if(projCoords.z > 1.0)
        return 0.0f;
    vec2 oneOverShadowDepthTextureSize = 1.0 / textureSize(Shadowmap, 0);
    float bias = 0.0000f;
    float shadow = 0;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offset = vec2(x, y) * oneOverShadowDepthTextureSize;
            float depth = texture(Shadowmap, projCoords.xy + offset).r; 
            shadow += (projCoords.z - bias) > depth ? 1.0 : 0.0;        
        }
    }

    return shadow /= 9.0;
}

void main() 
{
    float shadow = CalculateShadow();

    vec4 sampledColor = vec4(texture(bindlessTextures[colorTextureIndex], iTexCoord).rgb, 1.0f);

    vec3 normal = texture(bindlessTextures[normalTextureIndex], iTexCoord).xyz * 2.0 - 1.0;
    normal = normalize(iTBN * normal);

    vec3 ambient = 0.5 * sampledColor.rgb;

    //vec3 cameraPosition = vec3(InvView[3][0], InvView[3][1], InvView[3][2]);

    vec3 lightDir   = normalize(vec3(0.5, 1, -0.3));
    vec3 viewDir    = normalize(CameraPosition - iPosition);
    float diff = max(dot(-SunShadowDirection, normal), 0.0);
    vec3 diffuse = diff * sampledColor.rgb;

    vec3 reflectDir = reflect(SunShadowDirection, normal);
    vec3 halfwayDir = normalize(viewDir - SunShadowDirection);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 100.0);

    vec3 specular = vec3(0.5) * spec; // assuming bright white light color

    float light = (dot(lightDir, normal) + 2) / 3;

    outColor = vec4((ambient + diffuse + specular) * (1.0f - shadow * 0.5f), 1.0f);
}