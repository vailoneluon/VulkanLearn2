#version 450

struct InstanceSBOData{
	mat4 model;
}

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in mat4 instanceModelOffset;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint textureId;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 2, binding = 0) readonly buffer InstanceSBOBuffer{
	InstanceSBOData data[]; 
} instanceBuffer;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    uint textureId;
} pc;

void main() {
    gl_Position = ubo.proj * ubo.view * instanceBuffer.data[gl_InstanceIndex] * pc.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    textureId = pc.textureId;
}
