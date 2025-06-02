#version 330
in vec2 v_uv;

// Input textures
uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform sampler2D ssao_texture;
uniform sampler2D blurred_ssao_texture;

uniform int ssao_render_mode;
uniform bool use_blurred_ssao;

uniform float zNear;
uniform float zFar;

out vec4 frag_color;
  

void main()
{
    // Sample from G-Buffer textures
    vec3 color = texture(albedo_texture, v_uv).rgb;
    vec3 normal = texture(normal_texture, v_uv).rgb;
    float depth = texture(depth_texture, v_uv).r;
    float ssao = texture(ssao_texture, v_uv).r;
    float ssao_blurred = texture(blurred_ssao_texture, v_uv).r; 

    // Choose which AO to use for the final output
    float ao = use_blurred_ssao ? ssao_blurred : ssao;

    // Normal
    if(ssao_render_mode == 0){ 
        normal = normalize(normal);
        frag_color = vec4(normal, 1.0);

    // Albedo
    } else if(ssao_render_mode == 1){ 
        frag_color = vec4(color, 1.0);
    }
    // Depth
    else if(ssao_render_mode == 2){   
        // Convert from nonlinear [0,1] to linear depth
        float linear_depth = (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
        linear_depth = pow(linear_depth, 0.7);

        frag_color = vec4(vec3(1.0 - linear_depth), 1.0);
    }
    // SSAO
    else if(ssao_render_mode == 3){ 
        frag_color = vec4(vec3(ssao), 1.0);
    }
    // Blurred SSAO
    else if(ssao_render_mode == 4) {
        frag_color = vec4(vec3(ssao_blurred), 1.0);
    }
    // Final result: Albedo * AO
    else if (ssao_render_mode == 5) {
        frag_color = vec4(color.rgb * ao, 1.0);
    }
    // Default case: just output the color
    else {
        frag_color = vec4(color, 1.0); 
    }
}