#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor; // Optional: for vertex coloring
layout(location = 2) in vec3 inNormal; // Added normal attribute
layout(location = 3) in vec2 inTexCoord; // Adjusted location
layout(location = 4) in uint inBlockType; // New vertex attribute for texture array index

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat uint fragTexLayer; // Changed to flat uint

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection; // Changed from projectionView
    mat4 view;       // Added view matrix
} ubo;

layout(push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushConstants;

void main() {
    gl_Position = ubo.projection * ubo.view * pushConstants.modelMatrix * vec4(inPosition, 1.0); // Updated to use projection and view
    fragTexCoord = inTexCoord;
    fragTexLayer = inBlockType; // Use inBlockType for the texture layer
}
