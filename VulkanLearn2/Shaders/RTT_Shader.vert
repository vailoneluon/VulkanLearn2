#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint fragMaterialId;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragWorldNormal;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    uint materialId;
} pc;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragMaterialId = pc.materialId;

    // Calculate and pass world position and normal
    fragWorldPos = (pc.model * vec4(inPosition, 1.0)).xyz;
    // Correctly transform normal to world space
    fragWorldNormal = normalize(transpose(inverse(mat3(pc.model))) * inNormal);
}