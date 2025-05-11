#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoordLocal; // Received local UVs (0-1 for the sub-texture)

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textureAtlasSampler;

// Push constants are accessible here as well if defined for fragment stage in pipeline layout
layout(push_constant) uniform PushConstantData {
    mat4 modelMatrix; // Not used in frag, but part of the struct
    vec2 atlasUvMin;
    vec2 atlasUvMax;
} pushConstants;

void main() {
    // Transform local UVs to atlas UVs
    vec2 atlasTexCoord;
    atlasTexCoord.x = pushConstants.atlasUvMin.x + fragTexCoordLocal.x * (pushConstants.atlasUvMax.x - pushConstants.atlasUvMin.x);
    atlasTexCoord.y = pushConstants.atlasUvMin.y + fragTexCoordLocal.y * (pushConstants.atlasUvMax.y - pushConstants.atlasUvMin.y);

    // Sample the texture atlas and combine with vertex color
    outColor = texture(textureAtlasSampler, atlasTexCoord) * vec4(fragColor, 1.0);
}
