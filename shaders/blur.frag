#version 330 core

in vec2 v_uv;

// Input textures
uniform sampler2D ssao_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;

uniform int blur_type; // 0=Simple, 1=Bilateral, 2=Gaussian
uniform float blur_radius;
uniform vec2 viewport_size;

// Bilateral blur parameters
uniform float normal_threshold;
uniform float depth_threshold;

out vec4 frag_color;

const float PI = 3.14159265359;

// Gaussian weights for blur
float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * PI) * sigma);
}

// Simple box blur
vec4 simpleBlur() {
    vec2 texel_size = 1.0 / viewport_size;
    vec4 result = vec4(0.0);
    float total_weight = 0.0;
    
    // sample a square area around the pixel
    int radius = int(blur_radius);
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            vec2 sample_coord = v_uv + offset;
            
            // boundary check
            if (sample_coord.x >= 0.0 && sample_coord.x <= 1.0 && 
                sample_coord.y >= 0.0 && sample_coord.y <= 1.0) {
                result += texture(ssao_texture, sample_coord);
                total_weight += 1.0; // each sample has equal weight
            }
        }
    }
    
    return result / total_weight;
}

// Gaussian blur
vec4 gaussianBlur() {
    vec2 texel_size = 1.0 / viewport_size;
    vec4 result = vec4(0.0);
    float total_weight = 0.0;
    
    int radius = int(blur_radius);
    float sigma = blur_radius / 2.0;
    
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            vec2 sample_coord = v_uv + offset;
            
            // boundary check
            if (sample_coord.x >= 0.0 && sample_coord.x <= 1.0 && 
                sample_coord.y >= 0.0 && sample_coord.y <= 1.0) {
                
                // distance from center for gaussian weight
                float distance = length(vec2(float(x), float(y)));
                float weight = gaussian(distance, sigma);
                
                result += texture(ssao_texture, sample_coord) * weight;
                total_weight += weight;
            }
        }
    }
    
    return result / total_weight;
}

// Bilateral blur - preserves edges
vec4 bilateralBlur() {
    vec2 texel_size = 1.0 / viewport_size;
    vec4 center_ssao = texture(ssao_texture, v_uv);
    vec3 center_normal = texture(normal_texture, v_uv).xyz;
    float center_depth = texture(depth_texture, v_uv).r;
    
    vec4 result = vec4(0.0);
    float total_weight = 0.0;
    
    int radius = int(blur_radius);
    float sigma = blur_radius / 2.0;
    
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            vec2 sample_coord = v_uv + offset;
            
            // Check if the sample coordinate is within bounds
            if (sample_coord.x >= 0.0 && sample_coord.x <= 1.0 && 
                sample_coord.y >= 0.0 && sample_coord.y <= 1.0) {
                
                // Sample neighboring SSAO, normal, and depth
                vec4 sample_ssao = texture(ssao_texture, sample_coord);
                vec3 sample_normal = texture(normal_texture, sample_coord).xyz;
                float sample_depth = texture(depth_texture, sample_coord).r;
                
                // Spatial weight (Gaussian) - based on distance from center pixel
                float distance = length(vec2(float(x), float(y)));
                float spatial_weight = gaussian(distance, sigma);
                
                // Normal similarity weight - preserve edges where surface orientation changes
                // high weight for similar normals, low weight for different normals
                float normal_diff = dot(center_normal, sample_normal);
                float normal_weight = (normal_diff > normal_threshold) ? 1.0 : 0.1;
                
                // Depth similarity weight - preserve edges where depth changes
                // high weight for similar depths, low weight for different depths
                float depth_diff = abs(center_depth - sample_depth);
                float depth_weight = (depth_diff < depth_threshold) ? 1.0 : 0.1;

                // Combined weight - only blur pixels that are spatially close and geometrically similar
                float weight = spatial_weight * normal_weight * depth_weight;
                
                result += sample_ssao * weight;
                total_weight += weight;
            }
        }
    }
    
    if (total_weight > 0.0) {
        return result / total_weight;
    } else {
        return center_ssao;
    }
}

void main() {
    // Simple blur
    if (blur_type == 0) {
        frag_color = simpleBlur();
    }
    // Bilateral blur
    else if (blur_type == 1) {
        frag_color = bilateralBlur();
    }
    // Gaussian blur
    else if (blur_type == 2) {
        frag_color = gaussianBlur();
    }
    // Default case - simple blur
    else {
        frag_color = simpleBlur();
    }
}