#version 450

// IN from Vertex Shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint fragMaterialId;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragWorldNormal;

// OUT to G-Buffer attachments
layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;

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
    
    // Output to G-Buffer
    outAlbedo = texture(texSampler[diffuseTextureIndex], fragTexCoord);
    outNormal = vec4(normalize(fragWorldNormal), 1.0);
    outPosition = vec4(fragWorldPos, 1.0);
}