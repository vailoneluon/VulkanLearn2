#version 450

// IN from Vertex Shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint fragMaterialId;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragWorldNormal;
layout(location = 4) in vec3 fragTangent;

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
    // --- 1. Get Material Data ---
    MaterialData material = materialsBuffer.materials[fragMaterialId];
    
    // --- 2. Write Albedo and Position to G-Buffer ---
    uint diffuseTextureIndex = material.diffuseMapIndex;
    outAlbedo = texture(texSampler[diffuseTextureIndex], fragTexCoord);
    outPosition = vec4(fragWorldPos, 1.0);

    // --- 3. Calculate Final Normal from Normal Map ---
    
    // Get normal from normal map (in tangent space)
    uint normalMapIndex = material.normalMapIndex;
    vec3 tangentNormal = texture(texSampler[normalMapIndex], fragTexCoord).rgb;
    // Remap values from [0, 1] color range to [-1, 1] vector range
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

    // Construct the TBN matrix to transform from tangent space to world space
    // Re-normalize interpolated vectors for accuracy
    vec3 N = normalize(fragWorldNormal);
    vec3 T = normalize(fragTangent);
    // Use Gram-Schmidt process to ensure T and N are orthogonal
    T = normalize(T - dot(T, N) * N);
    // Calculate Bitangent B using cross product
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    // Transform normal from tangent space to world space
    vec3 finalNormal = normalize(TBN * tangentNormal);

    // Write final normal to G-Buffer
    outNormal = vec4(finalNormal, 1.0);
}