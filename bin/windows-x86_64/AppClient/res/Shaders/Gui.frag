#version 450

layout(location = 0) in vec2 iTexCoord;
layout(location = 1) in vec4 iColor;
layout(location = 2) in flat uint iTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D bindlessTextures[1024];

layout( push_constant ) uniform constants
{
	uint textureIndex;
};

void main()
{
    vec4 sampled = texture(bindlessTextures[iTexIndex], iTexCoord);
    outColor = sampled * iColor;
}