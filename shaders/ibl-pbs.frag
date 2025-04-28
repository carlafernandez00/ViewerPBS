#version 330
in vec3 v_normal;
in vec3 v_world_position;

uniform vec3 light;             // Light position
uniform vec3 camera_position;   // Camera position

// Material Properties
uniform vec3 fresnel;           // F0: Frenel 
uniform vec3 albedo; 
uniform float roughness;
uniform float metalness;

uniform samplerCube specular_map;
uniform samplerCube diffuse_map;
uniform sampler2D brdfLUT_map;

out vec4 frag_color;

const float MAX_REFLECTION_LOD = 7.0;

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
vec3 compute_light(vec3 N, vec3 R, vec3 V)
{
    vec3 F0 = mix(fresnel, albedo, metalness);
    float NdotV = clamp(dot(N,V), 0.0, 1.0);

    // Compute Diffuse and Specular Factors
    vec3 Ks = fresnel_schlick_roughness(NdotV, F0, roughness);
    vec3 Kd = (vec3(1.0) - Ks) * (1.0 - metalness);

    // Load Irradiance - Diffuse Term
    vec3 irradiance = texture(diffuse_map, N).rgb;
    vec3 diffuse    = irradiance * albedo;
    vec3 ambient    = Kd * diffuse; 
    
    // Load Specular
    vec3 prefiltered_color = textureLod(specular_map, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 env_BRDF = texture(brdfLUT_map, vec2(NdotV, roughness)).rg;
    vec3 specular = prefiltered_color * (Ks * env_BRDF.x + env_BRDF.y);

    // BRDF
    return ambient + specular;
}

void main (void) {
    // get vectors
    vec3 normal = normalize(v_normal);
    vec3 reflect_dir = normalize(reflect(-light_dir, normal));
    vec3 view_dir = normalize(camera_position - v_world_position);

    vec3 color = compute_light(normal, reflect_dir, view_dir);
    
    frag_color = vec4(color, 1.0);
}

