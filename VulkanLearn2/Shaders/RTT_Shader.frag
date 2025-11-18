#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint fragMaterialId;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler[256];

struct MaterialData {
    uint diffuseMapIndex;
    uint normalMapIndex;
    uint specularMapIndex;
    uint padding;
};

layout(set = 2, binding = 0) readonly buffer Materials {
    MaterialData materials[];
} materialsBuffer;

void main() {
    MaterialData material = materialsBuffer.materials[fragMaterialId];
    
    uint diffuseTextureIndex = material.diffuseMapIndex;
    
    outColor = texture(texSampler[diffuseTextureIndex], fragTexCoord);
}