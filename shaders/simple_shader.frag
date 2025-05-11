#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in flat uint textureArrayIndex;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor;
} ubo;

void main() {
    // Use the vertex color directly as RGB values
    // This uses the vertex's relative position which is passed as fragColor
    outColor = vec4(fragColor, 1.0);
}
