#version 450

layout(location = 0) in vec2 coord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorAttachment;

void main()
{
    vec2 uv = coord * 2.0 - 1.0;
    float dist = uv.x*uv.x + uv.y*uv.y;

    vec4 sampledColor = texture(colorAttachment, coord);
    //float depth = sampledColor.r;
    //outColor = vec4(vec3(depth), 1.0f);

    outColor = sampledColor;

}