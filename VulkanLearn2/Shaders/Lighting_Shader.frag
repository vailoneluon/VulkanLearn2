#version 450

// IN from Vertex Shader (fullscreen quad)
layout(location = 0) in vec2 inUV;

// OUT to the final lit image
layout(location = 0) out vec4 outColor;

// G-Buffer samplers (only gAlbedo will be used for now)
layout(set = 0, binding = 0) uniform sampler2D gAlbedo;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gPosition;

void main() {
    // For now, simply output the albedo color from the G-Buffer
    outColor = texture(gAlbedo, inUV);
}