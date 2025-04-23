#version 330
in vec3 v_position;
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_world_position;

uniform vec3 light;           // Light position
uniform vec3 camera_position; // Camera position

uniform sampler2D color_map;

out vec4 frag_color;

void main(void) {
    // light and object colors
    vec3 light_color = vec3 (1.0f);
    vec3 object_color = vec3(0.0f, 0.0f, 1.0f);
    vec3 base_color = vec3(texture(color_map, v_uv));

    // get vectors
    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(light - v_world_position);
    vec3 reflect_dir = normalize(reflect(-light_dir, normal));
    vec3 view_dir = normalize(camera_position - v_world_position);

    // ambient component
    float ambient_strength = 0.4f;
    vec3 ambient = ambient_strength * light_color;

    // diffuse component
    float NdotL = clamp(dot(normal, light_dir), 0.0, 1.0);
    vec3 diffuse = NdotL * light_color;

    // specular component 
    float specular_strength = 1.0f;
    float shininess = 45.0;
    float RdotV = clamp(dot(reflect_dir, view_dir), 0.0, 1.0);
    float spec = pow(RdotV, shininess);
    vec3 specular =  specular_strength * spec * light_color; 

    // total
    vec3 result_color = (ambient + diffuse + specular) * object_color;

    frag_color = vec4(result_color, 1.0);
}
