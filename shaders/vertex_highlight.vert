#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    // vec4 ambientLightColor; // Not used in this shader
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    // mat4 normalMatrix; // Not used in this shader
} push;

void main() {
    gl_Position = ubo.projection * ubo.view * push.modelMatrix * vec4(inPosition, 1.0);
    // Set the size of the points. Adjust as needed.
    // You might need to enable VK_SHADER_STAGE_VERTEX_BIT for shaderTessellationAndGeometryPointSize feature in Vulkan
    gl_PointSize = 15.0; 
}
