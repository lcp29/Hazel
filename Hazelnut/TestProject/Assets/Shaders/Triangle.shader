#version 460

#include "Library/Common.glsl"

vec3 vertices[3] = vec3[](
    vec3(0.0, -0.5, 0.0),
    vec3(-0.5, 0.5, 0.0),
    vec3(0.5, 0.5, 0.0)
);

vec2 uvs[3] = vec2[](
    vec2(0.5, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

layout (set = 1, binding = 0) uniform B
{
	int b;
} ub;

BeginPerMaterialProperties()
MaterialTextureSampler(color)
EndPerMaterialProperties()

#ifdef VERTEX_SHADER

layout (location = 0) out vec2 uv;

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
    uv = uvs[gl_VertexIndex];
}

#endif

#ifdef FRAGMENT_SHADER

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(GetMaterialTextureSampler(color), uv);
}

#endif