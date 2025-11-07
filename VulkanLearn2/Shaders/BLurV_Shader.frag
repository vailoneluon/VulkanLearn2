#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D horizontalBlurSampler;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    float texelOffsetY = 1.0 / textureSize(horizontalBlurSampler, 0).y;

    vec4 result = vec4(0.0);
    
    result += texture(horizontalBlurSampler, inUV) * weights[0];
    
    for(int i = 1; i < 5; ++i) {
        result += texture(horizontalBlurSampler, inUV + vec2(0.0, texelOffsetY * i)) * weights[i];
        
        result += texture(horizontalBlurSampler, inUV - vec2(0.0, texelOffsetY * i)) * weights[i];
    }
    
    outColor = result;
}