// Common Bindings

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout (std430, set = 0, binding = 0) uniform PerCameraInfo
{
    mat4x4 view;
    mat4x4 projection;
    mat4x4 viewProjection;
} perCameraInfo;

layout (set = 0, binding = 1) uniform texture2D bindlessTextures[65536];
layout (set = 0, binding = 2) uniform sampler bindlessSamplers[65536];
layout (set = 0, binding = 3) uniform sampler2D bindlessTextureSamplers[65536];

layout (push_constant) uniform PushConstants
{
    mat4x4 model;
    uint materialIndex;
    uint entityIndex;
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
    bindlessTextures[nonuniformEXT(properties[pushConstants.materialIndex].texture_##name)]

#define GetMaterialSampler(name) \
    bindlessSamplers[nonuniformEXT(properties[pushConstants.materialIndex].sampler_##name)]

#define GetMaterialTextureSampler(name) \
    bindlessTextureSamplers[nonuniformEXT(properties[pushConstants.materialIndex].combined_##name)]

#define GetMaterialProperty(name) \
    properties[pushConstants.materialIndex].name

#define BeginUserValues() \
    layout(set = 1, binding = 0) uniform UserUploadValues \
    {

#define EndUserValues() \
    } userUploadValues;

#define UserValue(type, name) \
    type name;

#define GetUserValue(name) \
    userUploadValues.name

mat4x4 GetViewProjectionMatrix()
{
    return perCameraInfo.viewProjection;
}

mat4x4 GetModelMatrix()
{
    return pushConstants.model;
}

uint GetEntityIndex()
{
    return pushConstants.entityIndex;
}
