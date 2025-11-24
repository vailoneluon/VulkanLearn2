#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// Input 1: Original Lit Scene (Linear HDR)
layout(set = 0, binding = 0) uniform sampler2D SceneSampler;
// Input 2: Blurred Bloom Texture (Linear HDR)
layout(set = 0, binding = 1) uniform sampler2D BrightSampler;

// --- CONFIGURATION ---
const float g_BloomStrength = 0.3;

// FXAA Quality Settings
const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0/8.0;
const float FXAA_REDUCE_MIN = 1.0/128.0;

// --- HELPER FUNCTIONS ---

// 1. ACES Tone Mapping (Standard Cinematic Curve)
vec3 StartACES(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

// 2. Core Composition Logic (Scene + Bloom + ToneMap)
// This function replaces the old main() logic.
// It returns the final LDR color at any given UV coordinate.
vec3 GetColor(vec2 uv) {
    vec3 sceneColor = texture(SceneSampler, uv).rgb;
    vec3 bloomColor = texture(BrightSampler, uv).rgb;
    
    // Additive blending
    vec3 bloomAdded = bloomColor * g_BloomStrength;
    vec3 result = sceneColor + bloomAdded;
    
    // Apply Tone Mapping to convert HDR to LDR
    return StartACES(result);
}

// 3. Luma Calculation
// Converts RGB to a single brightness value (Luminance)
// Needed for edge detection.
float Rgb2Luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

// --- MAIN FXAA ALGORITHM ---
void main() {
    // Calculate the size of a single texel
    vec2 texelSize = 1.0 / textureSize(SceneSampler, 0);

    // Sample the center pixel and its 4 diagonal neighbors
    vec3 rgbM  = GetColor(inUV);
    vec3 rgbNW = GetColor(inUV + (vec2(-1.0, -1.0) * texelSize));
    vec3 rgbNE = GetColor(inUV + (vec2( 1.0, -1.0) * texelSize));
    vec3 rgbSW = GetColor(inUV + (vec2(-1.0,  1.0) * texelSize));
    vec3 rgbSE = GetColor(inUV + (vec2( 1.0,  1.0) * texelSize));

    // Calculate Luma for all samples
    float lumaM  = Rgb2Luma(rgbM);
    float lumaNW = Rgb2Luma(rgbNW);
    float lumaNE = Rgb2Luma(rgbNE);
    float lumaSW = Rgb2Luma(rgbSW);
    float lumaSE = Rgb2Luma(rgbSE);

    // Find the maximum and minimum luma in the 3x3 area
    // This helps determine the local contrast.
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Calculate the sampling direction (where the edge is pointing)
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // Scale the direction to reduce artifacts
    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    // Clamp the direction vector
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * texelSize;

    // --- First Sampling Step (Close range) ---
    vec3 rgbA = (1.0/2.0) * (
        GetColor(inUV.xy + dir * (1.0/3.0 - 0.5)) +
        GetColor(inUV.xy + dir * (2.0/3.0 - 0.5)));
        
    // --- Second Sampling Step (Far range) ---
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        GetColor(inUV.xy + dir * (0.0/3.0 - 0.5)) +
        GetColor(inUV.xy + dir * (3.0/3.0 - 0.5)));
        
    float lumaB = Rgb2Luma(rgbB);

    // --- Final Selection ---
    // If the luma of the far sample (B) is outside the local contrast range (Min/Max),
    // it means we sampled too far (off the edge), so use the close sample (A).
    // Otherwise, use the smoother sample (B).
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        outColor = vec4(rgbA, 1.0);
    } else {
        outColor = vec4(rgbB, 1.0);
    }
}