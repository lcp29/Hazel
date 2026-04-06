// Common Bindings

layout (set = 0, binding = 0) uniform PerCameraInfo
{
    vec3 cameraPos;
} perCameraInfo;

layout (set = 0, binding = 1) uniform texture2D bindlessTextures[];
layout (set = 0, binding = 2) uniform sampler bindlessSamplers[];
layout (set = 0, binding = 3) uniform sampler2D bindlessTextureSamplers[];

layout (push_constant) uniform PushConstants
{
    mat4x4 view;
    mat4x4 projection;
    mat4x4 viewProjection;
    uint materialIndex;
} pushConstants;

// Per Material Property Wrappers

#define BeginPerMaterialProperties() \
    struct PerMaterialProperties \
    {

#define EndPerMaterialProperties() \
    }; \
    layout (std430, set = 2, binding = 0) readonly buffer PerMaterialPropertyBlock { PerMaterialProperties properties[]; };

#define MaterialProperty(type, name) \
    type name;

#define MaterialTexture(name) \
    uint texture_##name;

#define MaterialTextureSampler(name) \
    uint combined_##name;

#define MaterialSampler(name) \
    uint sampler_##name;

#define GetMaterialTexture(name) \
    bindlessTextures[properties[materialIndex].MaterialTexture(name)]

#define GetMaterialSampler(name) \
    bindlessSamplers[properties[materialIndex].MaterialSampler(name)]

#define GetMaterialTextureSampler(name) \
    bindlessTextureSamplers[properties[materialIndex].MaterialTextureSampler(name)]

#define GetMaterialProperty(name) \
    properties[materialIndex].name

vec3 GetCameraPos()
{
    return perCameraInfo.cameraPos;
}
