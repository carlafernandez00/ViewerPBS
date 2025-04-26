#version 330

in vec2 v_uv;

uniform int current_texture;
uniform sampler2D color_map;
uniform sampler2D roughness_map;
uniform sampler2D metalness_map;

out vec4 frag_color;

void main (void) {
    // Color Map
    if (current_texture == 0) {
        frag_color = texture(color_map, v_uv);
        } 
    // Roughness Map
    else if (current_texture == 1) {
        frag_color = texture(roughness_map, v_uv);
        } 
    // Metalness Map
    else if (current_texture == 2) {
        frag_color = texture(metalness_map, v_uv);
        } 
    // Default: gray color
    else {
        frag_color = vec4(0.72, 0.72, 0.72, 1.0);
    }
}
