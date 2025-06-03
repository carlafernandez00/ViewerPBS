#version 330

smooth in vec3 v_normal;
in vec2 v_uv;

uniform vec3 albedo; 
uniform bool use_textures;
uniform sampler2D color_map;

layout (location = 0) out vec4 frag_albedo;
layout (location = 1) out vec4 frag_normal;

void main(void) {
    vec3 normal = normalize(v_normal);

    // Output the albedo
    if (use_textures) {
        vec3 color = texture(color_map, v_uv).rgb;
        frag_albedo = vec4(color, 1.0);

    } else {
        frag_albedo = vec4(albedo, 1.0);
    }


    // Output the normal
    frag_normal = vec4(normal, 1.0);
}
