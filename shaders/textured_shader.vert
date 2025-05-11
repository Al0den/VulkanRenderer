#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord; // Local UVs for the texture (0-1 range)

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoordLocal; // Pass local UVs to fragment shader

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
} ubo;

// Push constants will include modelMatrix, and atlas UV transformation data
layout(push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    vec2 atlasUvMin;
    vec2 atlasUvMax;
} pushConstants;

void main() {
    gl_Position = ubo.projection * ubo.view * pushConstants.modelMatrix * vec4(position, 1.0);
    fragColor = color;
    fragTexCoordLocal = texCoord; // Pass the model's local UVs
}
