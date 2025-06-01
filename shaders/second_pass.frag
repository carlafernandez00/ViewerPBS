#version 330
in vec2 v_uv;

uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform int ssao_render_mode;

out vec4 frag_color;

void main()
{
    // Normal
    if(ssao_render_mode == 0){ 
        vec3 normal = texture(normal_texture, v_uv).rgb;
        frag_color = vec4(normal, 1.0);

    // Albedo
    } else if(ssao_render_mode == 1){ 
        vec3 color = texture(albedo_texture, v_uv).rgb;
        frag_color = vec4(color, 1.0);
    }
    // Depth
    else if(ssao_render_mode == 2){ 
        float depth = texture(depth_texture, v_uv).r;
        frag_color = vec4(depth, depth, depth, 1.0);
    }
    // Default case for unsupported render modes
    else {
        frag_color = vec4(1.0, 0.0, 1.0, 1.0); 
    }
}