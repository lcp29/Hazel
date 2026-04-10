#version 460

#extension GL_GOOGLE_include_directive : require

#include "Library/Common.glsl"

BeginPerMaterialProperties()
    MaterialTextureSampler(color)
EndPerMaterialProperties()

BeginUserValues()
    UserValue(float, lightStrength)
EndUserValues()

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_Normal;

void main()
{
    v_TexCoord = a_TexCoord;
    v_Normal = normalize(a_Normal);

    gl_Position = GetViewProjectionMatrix() * GetModelMatrix() * vec4(a_Position, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_Normal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 albedo = texture(GetMaterialTextureSampler(color), v_TexCoord).rgb;

    vec3 lightDir = normalize(vec3(0.0, 1.0, 0.0));
    float ndotl = max(dot(normalize(v_Normal), lightDir), 0.0);
    float irradiance = ndotl * GetUserValue(lightStrength);
    vec4 color = texture(GetMaterialTextureSampler(color), v_TexCoord);
    float ambient = 0.1;

    vec3 lighting = vec3(ambient + irradiance) * color.rgb;
    outColor = vec4(albedo, 1.0);
}

#endif
