#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneSampler;

const float threshold = 1; 

void main() {
    vec4 color = texture(sceneSampler, inUV);
    
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    if(brightness > threshold) {
        outColor = color;
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}