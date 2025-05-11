#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint fragTexLayer; // Changed to flat uint

layout(set = 1, binding = 0) uniform sampler2DArray textureAtlas;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textureAtlas, vec3(fragTexCoord, float(fragTexLayer)));
}
