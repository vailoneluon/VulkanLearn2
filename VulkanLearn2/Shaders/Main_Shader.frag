#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// Input 1: Original Lit Scene (Linear HDR)
layout(set = 0, binding = 0) uniform sampler2D SceneSampler;
// Input 2: Blurred Bloom Texture (Linear HDR)
layout(set = 0, binding = 1) uniform sampler2D BrightSampler;

// --- COMPOSITE CONFIGURATION ---
// Bloom Strength: Controls the intensity of the glow.
// - 0.04: Subtle (Cinematic/Anti-aliasing look)
// - 0.30: Medium (Recommended start)
// - 1.00: Strong (Dreamy/Neon look)
const float g_BloomStrength = 0.3;

// ACES Tone Mapping Function (Standard Cinematic Curve)
vec3 startACES(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

void main() {
    vec3 sceneColor = texture(SceneSampler, inUV).rgb;
    vec3 bloomColor = texture(BrightSampler, inUV).rgb;

    // 1. Apply Bloom Strength
    // Scale the bloom intensity before adding it to the scene.
    vec3 bloomAdded = bloomColor * g_BloomStrength;

    // 2. Additive Blending
    vec3 result = sceneColor + bloomAdded;

    // 3. Tone Mapping (HDR -> LDR)
    // CRITICAL: Tone mapping must be applied AFTER adding bloom.
    result = startACES(result);

    // 4. Gamma Correction
    // Since your Swapchain format is VK_FORMAT_B8G8R8A8_SRGB, 
    // the hardware handles gamma correction automatically.
    // If using UNORM format, uncomment: result = pow(result, vec3(1.0 / 2.2));
    
    outColor = vec4(result, 1.0);
}