#version 330
in vec3 v_normal;
in vec3 v_world_position;
in vec2 v_uv;

uniform vec3 light;             // Light position
uniform vec3 camera_position;   // Camera position
uniform bool gamma_correction;
uniform bool use_textures;

// Material Properties
uniform vec3 fresnel;           // F0: Frenel 

// Global Values
uniform vec3 albedo; 
uniform float roughness;
uniform float metalness;

// Material Maps
uniform sampler2D color_map;
uniform sampler2D roughness_map;
uniform sampler2D metalness_map;

uniform samplerCube weighted_specular_map;
uniform samplerCube diffuse_map;
uniform sampler2D brdfLUT_map;

out vec4 frag_color;


// BRDF FUNCTIONS
// Fresnel Schlick Approximation
// Now we don't use the halfway vector to determine the Fresnel response (influenced by the roughness of the surface),
// so we add the roughness term into the equation 
vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
} 
  
// rendering equation for one light
vec3 compute_light(vec3 N, vec3 R, vec3 V, float material_metalness, float material_roughness, vec3 material_albedo)
{
    vec3 F0 = mix(fresnel, material_albedo, material_metalness);
    float NdotV = clamp(dot(N,V), 0.0, 1.0);

    // Compute Diffuse and Specular Factors
    vec3 Ks = fresnel_schlick_roughness(NdotV, F0, material_roughness);
    vec3 Kd = (vec3(1.0) - Ks) * (1.0 - material_metalness);

    // Load Irradiance - Diffuse Term
    vec3 irradiance = texture(diffuse_map, N).rgb;
    vec3 diffuse    = irradiance * material_albedo;
    vec3 ambient    = Kd * diffuse; 
    
    // Load Specular
    float lod = material_roughness * 4.0; // convert from [0, 1] to [0, 4] (5 mip levels)
    vec3 prefiltered_color = textureLod(weighted_specular_map, R,  lod).rgb;
    vec2 env_BRDF = texture(brdfLUT_map, vec2(NdotV, material_roughness)).rg;
    vec3 specular = prefiltered_color * (Ks * env_BRDF.x + env_BRDF.y);

    // BRDF
    return ambient + specular;
}

void main (void) {
    // get vectors
    vec3 normal = normalize(v_normal);
    vec3 view_dir = normalize(camera_position - v_world_position);
    vec3 reflect_dir = normalize(reflect(-view_dir, normal));

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

    vec3 color = compute_light(normal, reflect_dir, view_dir, material_metalness, material_roughness, material_albedo);
    
    // Gamma correction
    if (gamma_correction) {
        color = pow(color, vec3(1.0 / 2.2));
    }

    frag_color = vec4(color, 1.0);
}

