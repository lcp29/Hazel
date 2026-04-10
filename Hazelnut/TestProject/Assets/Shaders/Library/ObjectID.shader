#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common.glsl"

BeginPerMaterialProperties()
    MaterialProperty(float, empty)
EndPerMaterialProperties()

BeginUserValues()
    UserValue(float, empty)
EndUserValues()

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;

void main()
{
    gl_Position = GetViewProjectionMatrix() * GetModelMatrix() * vec4(a_Position, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out int outColor;

void main()
{
    outColor = int(GetEntityIndex());
}

#endif
