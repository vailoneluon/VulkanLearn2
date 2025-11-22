#version 450

// IN from Vertex Shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint fragMaterialId;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragWorldNormal;
layout(location = 4) in vec3 fragTangent;

// OUT to G-Buffer attachments
layout(location = 0) out vec4 outAlbedoRoughness;   // .rgb = Albedo, .a = Roughness
layout(location = 1) out vec4 outNormalMetallic;    // .rgb = Normal, .a = Metallic
layout(location = 2) out vec4 outPositionAO;        // .rgb = Position, .a = Ambient Occlusion

// Bindless texture array
layout(set = 0, binding = 0) uniform sampler2D texSampler[256];

// Material data SSBO
struct MaterialData {
    uint diffuseMapIndex;
    uint normalMapIndex;
    uint specularMapIndex;  // Kept for alignment, can be repurposed
    uint roughnessMapIndex;
    uint metallicMapIndex;
    uint occlusionMapIndex;
    uint padding1;
    uint padding2;
};

layout(set = 2, binding = 0) readonly buffer Materials {
    MaterialData materials[];
} materialsBuffer;

void main() {
    // --- 1. Get Material Data ---
    MaterialData material = materialsBuffer.materials[fragMaterialId];
    
    // --- 2. Sample PBR Textures ---
    // Sample Albedo
    vec3 albedo = texture(texSampler[material.diffuseMapIndex], fragTexCoord).rgb;

    // Sample PBR maps according to the ORM standard (Occlusion = R, Roughness = G, Metallic = B)
    // We sample from the specified texture index for each property.
    // If they are packed into one texture, the indices in MaterialData should all point to that same texture.
	float roughness = texture(texSampler[material.roughnessMapIndex], fragTexCoord).g; // Roughness from Green channel
    
	float metallic  = texture(texSampler[material.metallicMapIndex], fragTexCoord).b;  // Metallic from Blue channel
    
	float ao        = texture(texSampler[material.occlusionMapIndex], fragTexCoord).r;   // AO from Red channel

    // --- 3. Calculate Final Normal from Normal Map ---
    vec3 tangentNormal = texture(texSampler[material.normalMapIndex], fragTexCoord).rgb;
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0); // Remap from [0,1] to [-1,1]

    vec3 N = normalize(fragWorldNormal);
    vec3 T = normalize(fragTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    vec3 finalNormal = normalize(TBN * tangentNormal);

    // --- 4. Write to G-Buffer Attachments ---
    outAlbedoRoughness   = vec4(albedo, roughness);
    outNormalMetallic    = vec4(finalNormal, metallic);
    outPositionAO        = vec4(fragWorldPos, ao);
}