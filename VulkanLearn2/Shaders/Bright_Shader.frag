#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D sceneSampler;

// --- AAA BLOOM CONFIGURATION ---
// 1. Threshold: Pixels brighter than this will contribute to bloom.
//    Recommended: 1.0 - 1.5 for your current setup.
const float g_Threshold = 1.2; 

// 2. Soft Knee: Controls the transition smoothness.
//    Prevents a harsh cutoff line between bloomed and non-bloomed areas.
const float g_SoftKnee = 0.5;

// 3. Max Brightness Clamp: ANTI-FIREFLY / STABILITY
//    Clamps the maximum input luminance to 10.0.
//    This prevents single extremely bright pixels (specular artifacts/fireflies)
//    from ruining the entire bloom effect.
const float g_MaxBrightness = 10.0; 

// Standard Rec.709 Luma calculation
float SafeLuma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void main() {
    vec3 color = texture(sceneSampler, inUV).rgb;

    // --- STAGE 1: Pre-Clamp (Anti-Firefly) ---
    float luma = SafeLuma(color);
    // If pixel is insanely bright (e.g., specular artifact), clamp it down.
    if (luma > g_MaxBrightness) {
        color *= (g_MaxBrightness / luma);
    }

    // --- STAGE 2: Soft Thresholding ---
    float brightness = SafeLuma(color);

    // Calculate Soft Knee (Quadratic curve)
    float knee = g_Threshold * g_SoftKnee;
    float soft = brightness - g_Threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);

    // Calculate Contribution
    float contribution = max(soft, brightness - g_Threshold);
    contribution /= max(brightness, 0.00001); // Normalize to prevent color shift

    // Result: Scale original color by contribution
    outColor = vec4(color * contribution, 1.0);
}