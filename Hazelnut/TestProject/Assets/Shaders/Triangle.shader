#version 460

#extension GL_GOOGLE_include_directive : require

#include "Library/Common.glsl"

BeginPerMaterialProperties()
    MaterialProperty(vec4, color)
EndPerMaterialProperties()

layout(set = 1, binding = 0) uniform UserUploadValues
{
    int a;
} userUploadValues;

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_Normal;

void main()
{
    const vec2 positions[3] = vec2[](
        vec2(-0.5, -0.5),
        vec2(0.0, 0.5),
        vec2(0.5, -0.5)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_Normal;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = GetMaterialProperty(color);
}

#endif
