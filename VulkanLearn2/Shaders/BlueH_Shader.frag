#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D brightSampler;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    float texelOffsetX = 1.0 / textureSize(brightSampler, 0).x;

    vec4 result = vec4(0.0);
    
    result += texture(brightSampler, inUV) * weights[0];
    
    for(int i = 1; i < 5; ++i) {
        result += texture(brightSampler, inUV + vec2(texelOffsetX * i, 0.0)) * weights[i];
        
        result += texture(brightSampler, inUV - vec2(texelOffsetX * i, 0.0)) * weights[i];
    }
    
    outColor = result;
}