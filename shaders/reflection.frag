#version 330

in vec3 v_normal;
in vec3 v_world_position;

uniform vec3 camera_position; 
uniform samplerCube specular_map;

out vec4 frag_color;

void main (void) {
    vec3 view_dir = normalize(v_world_position - camera_position);

    vec3 normal = normalize(v_normal);
    vec3 reflect_dir = normalize(reflect(view_dir, normal));

    frag_color = texture(specular_map, reflect_dir);
}
