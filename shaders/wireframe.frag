#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // Output a solid color for the wireframe (e.g., white)
    outColor = vec4(0, 0, 0, 1.0);
}
