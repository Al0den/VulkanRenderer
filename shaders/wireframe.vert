#version 450

layout(location = 0) in vec3 position;
// Unused attributes, but kept for compatibility with existing vertex structure if needed
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix; // Unused in this shader, but part of the push constant block
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

void main() {
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * push.modelMatrix * vec4(position, 1.0);
}
