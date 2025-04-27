#version 330
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_world_position;

uniform vec3 light;           // Light position
uniform vec3 camera_position; // Camera position
uniform vec3 fresnel;         // F0: Frenel 

// Material Properties
uniform bool use_textures;

// Global Values
uniform vec3 albedo; 
uniform float roughness;
uniform float metalness;

// Material Maps
uniform sampler2D color_map;
uniform sampler2D roughness_map;
uniform sampler2D metalness_map;

out vec4 frag_color;

const float PI = 3.14159265358979323846;
const float RECIPROCAL_PI = 0.3183098861837697;

// BRDF FUNCTIONS
// Fresnel Schlick Approximation
vec3 fresnel_schlick(vec3 F0, float  LdotH){
    return F0 + (1.0 - F0) * pow(1.0 - LdotH, 5.0);
}

// Lambertian Diffuse BRDF
vec3 lambert(vec3 albedo){
    return albedo * RECIPROCAL_PI;
}

// GGX Distribution Function
float D_GGX(float alphaSqr, float NdotH){
    float NdotH2 = NdotH * NdotH;
    float f = NdotH2 * (alphaSqr - 1.0) + 1.0;

    return alphaSqr / (PI * f * f);
}

// WalterEtAl Distribution Function
float WalterEtAl(float alphaSqr, float NdotVec){
    float NdotVecSqr = NdotVec*NdotVec;

    return 2/(1 + sqrt(1 + alphaSqr * (1-NdotVecSqr)/(NdotVecSqr)));
}

// Smith Model
float G(float alphaSqr, float NdotL, float NdotV)
{
    return WalterEtAl(alphaSqr, NdotV) * WalterEtAl(alphaSqr, NdotL);
}
  
// PBR for one light
vec3 compute_PBR(vec3 L, vec3 N, float material_metalness, float material_roughness, vec3 material_albedo, vec3 light_color){
    // Compute vectors
    vec3 V = normalize(camera_position - v_world_position);
    vec3 H = normalize(L + V);
    float NdotH = clamp(dot(N,H), 0.0, 1.0);
    float NdotV = clamp(dot(N,V), 0.0, 1.0);
    float LdotH = clamp(dot(L,H), 0.0, 1.0);
    float NdotL = clamp(dot(N,L), 0.0, 1.0);

    // Compute Diffuse and Specular Factors
    vec3 f0 = mix(fresnel, material_albedo, material_metalness);
    vec3 Ks = fresnel_schlick(f0, LdotH);
    vec3 Kd = (vec3(1.0) - Ks) * (1.0 - material_metalness);

    float alpha = material_roughness * material_roughness;
    float alphaSqr = (alpha * alpha) + 1e-6;

    // Diffuse BRDF
    vec3 Fd = lambert(material_albedo);
    vec3 diffuse = Kd * Fd;
    
    // Specular BRDF
    float D_GGX = D_GGX(alphaSqr, NdotH);
    float G = G(alphaSqr, NdotL, NdotV);
    vec3 specular = (D_GGX * G * Ks) / (4.0 * NdotL * NdotV + 1e-6);

    // BRDF
    vec3 direct_light = diffuse + specular;

    vec3 total_light = direct_light * light_color * NdotL;
    return total_light;
}

void main (void) {
    vec3 light_color = vec3 (1.0f);
    vec3 ambient_light = vec3(0.2f); 

    // get vectors
    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(light - v_world_position);

    // Get material properties
    vec3 material_albedo;
    float material_roughness;
    float material_metalness;

    if (use_textures) {
        material_albedo = texture(color_map, v_uv).xyz;
        material_roughness = texture(roughness_map, v_uv).x;
        material_metalness = texture(metalness_map, v_uv).x;
    }
    else{
        material_albedo = albedo;
        material_roughness = roughness;
        material_metalness = metalness;
    }

    vec3 total_light = compute_PBR(light_dir, normal, material_metalness, material_roughness, material_albedo, light_color);

    float NdotL = clamp(dot(normal, light_dir), 0.0, 1.0);
    vec3 color = ambient_light * material_albedo;
    color += total_light;
    frag_color = vec4(color, 1.0);
}
