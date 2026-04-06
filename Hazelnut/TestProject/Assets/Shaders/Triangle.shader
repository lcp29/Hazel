#version 460

#include "Triangle.glsl"

vec3 vertices[3] = vec3[](
    vec3(0.0, -0.5, 0.0),
    vec3(-0.5, 0.5, 0.0),
    vec3(0.5, 0.5, 0.0)
);

struct Te
{
	int t;
};

layout (set = 0, binding = 0) uniform A
{
	int a;
} ua;

layout (set = 1, binding = 0) uniform B
{
	int b;
} ub;


layout (std430, set = 2, binding = 0) readonly buffer MaterialProperties
{
	Te ta[];
};

#ifdef VERTEX_SHADER

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = color; // Orange color}
}

#endif