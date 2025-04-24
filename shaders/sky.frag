#version 330
in vec3 v_position;

uniform vec3 camera_position; 
uniform samplerCube specular_map;

out vec4 frag_color;

void main (void) {
    frag_color = texture(specular_map, v_position);
}
