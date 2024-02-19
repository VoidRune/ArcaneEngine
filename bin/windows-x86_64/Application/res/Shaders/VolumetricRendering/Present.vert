#version 450 core
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) out vec2 coord;

vec4 vertices[4] = vec4[](
    vec4( -1, -1, 0, 0 ),
    vec4(  1, -1, 1, 0 ),
    vec4(  1,  1, 1, 1 ),
    vec4( -1,  1, 0, 1 )
    //vec4(  1,  1, 1, 1 ),
    //vec4( -1, -1, 0, 0 )
);

void main() 
{
    vec4 vertex = vertices[gl_VertexIndex];
	gl_Position = vec4(vertex.xy, 0.0, 1.0);
	coord = vertex.zw;
}