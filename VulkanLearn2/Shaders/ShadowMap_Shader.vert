#version 450

// Input attributes matching Vertex::GetAttributeDesc()
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

// Push constants matching struct ShadowMapPushConstantData in VulkanTypes.h
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 lightSpaceMatrix;
} pc;

void main() {
    // Transform vertex position to light clip space
    gl_Position = pc.lightSpaceMatrix * pc.model * vec4(inPosition, 1.0);
}