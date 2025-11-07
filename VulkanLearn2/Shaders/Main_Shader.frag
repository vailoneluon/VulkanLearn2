#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D SceneSampler;
layout(set = 0, binding = 1) uniform sampler2D BrightSampler;

void main() {
	vec4 sceneColor = texture(SceneSampler, inUV);
	vec4 brightColor = texture(BrightSampler, inUV);

    outColor = sceneColor + brightColor;
}