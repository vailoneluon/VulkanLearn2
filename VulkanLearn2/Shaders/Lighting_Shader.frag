#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// =========================================================================
// SET 0: G-BUFFER INPUTS
// =========================================================================
layout(set = 0, binding = 0) uniform sampler2D gAlbedoSpec;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gPosition;

// =========================================================================
// SET 1: SCENE LIGHTS (SSBO)
// =========================================================================
struct Light {
    vec4 position;   // xyz: Pos, w: Type (0: Dir, 1: Point, 2: Spot)
    vec4 direction;  // xyz: Dir, w: Range
    vec4 color;      // rgb: Color, w: Intensity
    vec4 params;     // x: Inner, y: Outer, z: ShadowIdx, w: Radius
};

layout(std430, set = 1, binding = 0) readonly buffer LightBuffer {
    Light lights[];
} lightBuffer;

// =========================================================================
// SET 2: CAMERA DATA (UBO)
// =========================================================================
layout(set = 2, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    float padding;
} ubo;

const float PI = 3.14159265359;

// =========================================================================
// PBR FUNCTIONS
// =========================================================================

// 1. Normal Distribution Function (NDF) - Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); // Prevent divide by zero
}

// 2. Geometry Function - Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. Fresnel Equation - Fresnel-Schlick
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Attenuation function (Inverse Square Law)
float CalculateAttenuation(float distance, float range) {
    if(distance >= range) return 0.0;
    // Simple linear falloff for performance, or standard inverse square:
    // float attenuation = 1.0 / (distance * distance);
    // Adjusted to reach 0 at range:
    return clamp(1.0 - (distance / range), 0.0, 1.0); 
}

void main() 
{
    // --- 1. Get data from G-Buffer ---
    vec3 rawNormal = texture(gNormal, inUV).rgb;

    // Check for background pixels by testing the length of the raw normal vector.
    // In GeometryPass, background pixels should have a normal of (0,0,0).
    if (length(rawNormal) < 0.01) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0); // Output black background
        return;
    }

    // Now that we know it's not a zero vector, we can safely normalize it.
    vec3 N = normalize(rawNormal);
    
    vec3 WorldPos = texture(gPosition, inUV).rgb;
    vec3 Albedo = pow(texture(gAlbedoSpec, inUV).rgb, vec3(2.2)); // sRGB to Linear

    // --- PBR Parameters (Hardcoded) ---
    float metallic = 0.1;
    float roughness = 0.3;
    float ao = 1.0;

    vec3 V = normalize(ubo.viewPos - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, Albedo, metallic);
              
    vec3 Lo = vec3(0.0);

    uint numLights = lightBuffer.lights.length();
    for(uint i = 0; i < numLights; ++i) {
        Light light = lightBuffer.lights[i];
        int type = int(light.position.w);

        vec3 L;
        float attenuation = 1.0;
        vec3 lightColor = light.color.rgb * light.color.w;

        if (type == 0) { // Directional
            L = normalize(-light.direction.xyz);
        } else { // Point & Spot
            L = normalize(light.position.xyz - WorldPos);
            float distance = length(light.position.xyz - WorldPos);
            attenuation = CalculateAttenuation(distance, light.direction.w);

            if (type == 2) { // Spot
                float theta = dot(L, normalize(-light.direction.xyz));
                float epsilon = light.params.x - light.params.y;
                float intensity = clamp((theta - light.params.y) / epsilon, 0.0, 1.0);
                attenuation *= intensity;
            }
        }

        if (attenuation <= 0.0) continue;

        vec3 H = normalize(V + L);
        vec3 radiance = lightColor * attenuation;

        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  

        float NdotL = max(dot(N, L), 0.0);        
        Lo += (kD * Albedo / PI + specular) * radiance * NdotL; 
    }   
    
    vec3 ambient = vec3(0.03) * Albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0)); // Reinhard Tonemapping
    color = pow(color, vec3(1.0 / 2.2)); // Gamma Correction

    outColor = vec4(color, 1.0);
}