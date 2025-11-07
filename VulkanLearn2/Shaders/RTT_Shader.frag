#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint textureId;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler[256];

void main() {
    outColor = texture(texSampler[textureId], fragTexCoord);
}
