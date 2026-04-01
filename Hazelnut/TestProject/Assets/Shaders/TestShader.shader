#version 450

// set = 2, binding = 0: numeric UBO only
layout(set = 2, binding = 0) uniform MaterialParams
{
    float exposure;
    float mixFactor;
    int   useCombined;
    int   padding0; // pad to 16-byte alignment if you want safer host-side packing
} uParams;

// set = 2, binding > 0: only sampler/image/combined sampler image
layout(set = 2, binding = 1) uniform sampler2D uCombinedTex; // combined image sampler
layout(set = 2, binding = 2) uniform texture2D uSeparateTex; // separate image
layout(set = 2, binding = 3) uniform sampler   uSeparateSampler; // separate sampler

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 vUV;

void main()
{
    vUV = inUV;
    gl_Position = vec4(inPosition, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 c0 = texture(uCombinedTex, vUV);
    vec4 c1 = texture(sampler2D(uSeparateTex, uSeparateSampler), vUV);

    vec4 color = (uParams.useCombined != 0) ? c0 : c1;
    color.rgb *= uParams.exposure;

    outColor = color;
}

#endif