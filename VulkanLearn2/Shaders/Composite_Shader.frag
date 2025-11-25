#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// Input 1: Original Lit Scene (Linear HDR)
layout(set = 0, binding = 0) uniform sampler2D SceneSampler;
// Input 2: Blurred Bloom Texture (Linear HDR)
layout(set = 0, binding = 1) uniform sampler2D BrightSampler;

// =========================================================================
// CONFIGURATION
// =========================================================================

const float g_BloomStrength = 0.3;

// --- FXAA SETTINGS (PRESET: HIGH QUALITY / LOW BLUR) ---
// Reduced span max to limit blur radius
const float FXAA_SPAN_MAX = 4.0;
// Increased reduce mul to preserve texture details
const float FXAA_REDUCE_MUL = 1.0/4.0;
// Threshold to skip processing dark areas
const float FXAA_REDUCE_MIN = 1.0/64.0;

// --- SHARPENING SETTINGS (CAS) ---
// Strength of sharpening (0.0 = off, 1.0 = max). Recommended: 0.3 - 0.5
const float g_Sharpness = 0.8;

// =========================================================================
// HELPER FUNCTIONS
// =========================================================================

// 1. ACES Tone Mapping
vec3 StartACES(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

// 2. Luma Calculation
float Rgb2Luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

// 3. Core Fetch Logic (Scene + Bloom + ToneMap)
// Returns the final LDR color at a specific UV coordinate
vec3 FetchColor(vec2 uv) {
    vec3 sceneColor = texture(SceneSampler, uv).rgb;
    vec3 bloomColor = texture(BrightSampler, uv).rgb;
    
    vec3 result = sceneColor + (bloomColor * g_BloomStrength);
    return StartACES(result);
}

// 4. Simple CAS (Contrast Adaptive Sharpening) / Unsharp Mask
// Applies local sharpening to the center pixel
vec3 FetchColorSharpened(vec2 uv, vec2 texelSize) {
    // Sample center and 4 neighbors
    vec3 c  = FetchColor(uv);
    vec3 cN = FetchColor(uv + vec2(0.0, -1.0) * texelSize);
    vec3 cS = FetchColor(uv + vec2(0.0,  1.0) * texelSize);
    vec3 cW = FetchColor(uv + vec2(-1.0, 0.0) * texelSize);
    vec3 cE = FetchColor(uv + vec2( 1.0, 0.0) * texelSize);

    // Contrast calculation for adaptive sharpening
    float minG = min(Rgb2Luma(c), min(min(Rgb2Luma(cN), Rgb2Luma(cS)), min(Rgb2Luma(cW), Rgb2Luma(cE))));
    float maxG = max(Rgb2Luma(c), max(max(Rgb2Luma(cN), Rgb2Luma(cS)), max(Rgb2Luma(cW), Rgb2Luma(cE))));
    
    // Simplified sharpening logic for single-pass shader
    // Boosts the center pixel based on difference from neighbors
    return c + (c - (cN + cS + cW + cE) * 0.25) * g_Sharpness;
}

// =========================================================================
// MAIN SHADER (FXAA + SHARPENING)
// =========================================================================
void main() {
    vec2 texelSize = 1.0 / textureSize(SceneSampler, 0);

    // --- STAGE 1: SAMPLING ---
    // Use sharpened color for the center sample (rgbM) to restore detail
    // Use normal color for neighbors to save performance
    
    vec3 rgbM  = FetchColorSharpened(inUV, texelSize); 
    vec3 rgbNW = FetchColor(inUV + (vec2(-1.0, -1.0) * texelSize));
    vec3 rgbNE = FetchColor(inUV + (vec2( 1.0, -1.0) * texelSize));
    vec3 rgbSW = FetchColor(inUV + (vec2(-1.0,  1.0) * texelSize));
    vec3 rgbSE = FetchColor(inUV + (vec2( 1.0,  1.0) * texelSize));

    // --- STAGE 2: LUMINANCE & CONTRAST ---
    float lumaM  = Rgb2Luma(rgbM);
    float lumaNW = Rgb2Luma(rgbNW);
    float lumaNE = Rgb2Luma(rgbNE);
    float lumaSW = Rgb2Luma(rgbSW);
    float lumaSE = Rgb2Luma(rgbSE);

    // Find local min/max luma
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // --- STAGE 3: EDGE DETECTION ---
    // Determine edge direction (vertical or horizontal)
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // Reduce direction influence on low contrast areas (texture details)
    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    // Clamp direction to avoid sampling too far
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * texelSize;

    // --- STAGE 4: BLENDING ---
    // Sample along the edge direction
    vec3 rgbA = (1.0/2.0) * (
        FetchColor(inUV.xy + dir * (1.0/3.0 - 0.5)) +
        FetchColor(inUV.xy + dir * (2.0/3.0 - 0.5)));
        
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        FetchColor(inUV.xy + dir * (0.0/3.0 - 0.5)) +
        FetchColor(inUV.xy + dir * (3.0/3.0 - 0.5)));
        
    float lumaB = Rgb2Luma(rgbB);

    // Check if Sample B is outside the local contrast range (overshoot)
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        outColor = vec4(rgbA, 1.0); // Use closer samples
    } else {
        outColor = vec4(rgbB, 1.0); // Use smoother samples
    }
}