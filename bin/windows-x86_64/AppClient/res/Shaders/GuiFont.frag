#version 450

layout(location = 0) in vec2 iTexCoord;
layout(location = 1) in vec4 iColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D bindlessTextures[1024];

layout( push_constant ) uniform constants
{
	uint fontTextureIndex;
};

const float pxRange = 2.0;

float median(vec3 rgb) {
    return max(min(rgb.r, rgb.g), min(max(rgb.r, rgb.g), rgb.b));
}

float screenPxRange() {
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(bindlessTextures[fontTextureIndex], 0).xy);
    vec2 screenTexSize = vec2(1.0) / iTexCoord;
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}


void main()
{
    vec4 bgColor = vec4(iColor.rgb, 0);
    vec4 fgColor = iColor;

    vec3 msd = texture(bindlessTextures[fontTextureIndex], iTexCoord).rgb;
    float sd = median(msd);
    float scrPxDist = screenPxRange()*(sd - 0.5);
    float opacity = clamp(scrPxDist + 0.5, 0.0, 1.0);

    float edge = 0.4;
    float borderLen = 0.35;
    float borderBlend = (opacity - edge) / borderLen;
    if(borderBlend <= 0.0)
    {
        discard;
    }

    outColor = mix(bgColor, fgColor, min(borderBlend, 1));
}