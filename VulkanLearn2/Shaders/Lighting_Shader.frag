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
// SET 1: SCENE LIGHTS & SHADOWS
// =========================================================================
struct Light {
    vec4 position;       // xyz: Pos, w: Type (0: Dir, 1: Point, 2: Spot)
    vec4 direction;      // xyz: Dir, w: Range
    vec4 color;          // rgb: Color, w: Intensity
    vec4 params;         // x: Inner, y: Outer, z: ShadowIdx, w: Radius
    mat4 lightSpaceMatrix;
};

layout(std430, set = 1, binding = 0) readonly buffer LightBuffer {
    Light lights[];
} lightBuffer;

// Bindless Shadow Maps array
// Using sampler2DShadow for hardware PCF/comparison
layout(set = 1, binding = 1) uniform sampler2DShadow shadowMaps[256]; 

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

// Attenuation function (Inverse Square Law with Windowing)
float CalculateAttenuation(float distance, float range) {
    if(distance >= range) return 0.0;
    
    float attenuation = 1.0 / (distance * distance + 1.0);
    float distDivRange = distance / range;
    float distDivRange4 = distDivRange * distDivRange * distDivRange * distDivRange; // ^4
    float window = clamp(1.0 - distDivRange4, 0.0, 1.0);
    
    return attenuation * window * window;
}

// =========================================================================
// SHADOW CALCULATION
// =========================================================================
float CalculateShadow(vec4 fragPosLightSpace, int shadowIdx, vec3 normal, vec3 lightDir)
{
    // 1. Perspective divide (standard for projection)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 2. Transform to [0,1] range
    // Vulkan's Z is [0,1], but XY is [-1,1] in clip space.
    // However, the depth texture coordinate system is [0,1].
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // 3. Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 1.0;

    // 4. Calculate Bias to prevent Shadow Acne
    // Bias increases as the slope (angle) increases.
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    // 5. Sample Shadow Map
    // 'texture' on a sampler2DShadow automatically performs the depth comparison.
    // It takes a vec3(u, v, d) where 'd' is the reference value to compare against.
    // Returns 1.0 if (d < storedDepth), 0.0 otherwise (or interpolated if filtering is on).
    // We subtract bias from our depth to fix acne.
    float shadow = texture(shadowMaps[shadowIdx], vec3(projCoords.xy, projCoords.z - bias));

    return shadow;
}

void main() 
{
    // --- 1. Unpack data from G-Buffer ---
    vec4 albedoRoughnessSample = texture(gAlbedoSpec, inUV);
    vec4 normalMetallicSample  = texture(gNormal, inUV);
    vec4 positionAOSample      = texture(gPosition, inUV);

    // Check for invalid normal/background (assuming normal length is ~1.0 for valid pixels)
    if (length(normalMetallicSample.rgb) < 0.01) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0); // Background color
        return;
    }

    // --- 2. Assign PBR material properties ---
    vec3 Albedo     = albedoRoughnessSample.rgb; 
    float roughness = max(albedoRoughnessSample.a, 0.05); // Clamp roughness to prevent artifacts

    vec3 N          = normalize(normalMetallicSample.rgb);
    float metallic  = normalMetallicSample.a;
    vec3 WorldPos   = positionAOSample.rgb;
    float ao        = positionAOSample.a;

    // --- 3. Prepare vectors for lighting ---
    vec3 V = normalize(ubo.viewPos - WorldPos);
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, Albedo, metallic);
              
    vec3 Lo = vec3(0.0);

    // --- 4. Loop through all lights ---
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

        // Optimization: skip if light has no effect
        if (attenuation <= 0.0) continue;

        // --- Shadow Calculation ---
        float shadowVisibility = 1.0; // Default to fully lit (no shadow)
        
        // Check if light has a shadow map assigned (index > -0.5)
        if (light.params.z > -0.5) {
            int shadowIdx = int(light.params.z);
            vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(WorldPos, 1.0);
            shadowVisibility = CalculateShadow(fragPosLightSpace, shadowIdx, N, L);
        }

        // --- PBR Calculation ---
        vec3 H = normalize(V + L);
        vec3 radiance = lightColor * attenuation;

        // Cook-Torrance BRDF
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
        
        // Combine everything
        // Shadow only affects the direct light contribution (Diffuse + Specular)
        Lo += (kD * Albedo / PI + specular) * radiance * NdotL * shadowVisibility;
    }   
    
    // --- 5. Final Assembly ---
    vec3 ambient = vec3(0.03) * Albedo * ao;
    vec3 color = ambient + Lo;

    outColor = vec4(color, 1.0);
}