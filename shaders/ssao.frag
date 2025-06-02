#version 330 core

in vec2 v_uv;

// Input textures
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform sampler2D noise_texture;

// SSAO parameters
uniform int num_directions;
uniform int samples_per_direction;
uniform float sample_radius;
uniform mat4 projection;
uniform vec2 viewport_size;
uniform vec2 noise_scale;

// Camera parameters
uniform float zNear;
uniform float zFar;
uniform float fov;

// Randomization controls
uniform bool use_randomization;
uniform float bias_angle;

out vec4 frag_color;

const float PI = 3.14159265359;

// Reconstruct position from depth and texture coordinates in eye space
vec3 reconstructPosition(vec2 texCoord, float depth) {
    float z_ndc = depth * 2.0 - 1.0;                                               // Convert from [0,1] depth to NDC z
    float z_eye = (2.0 * zNear * zFar) / (zFar + zNear - z_ndc * (zFar - zNear));  // Convert NDC z to eye space z
    vec2 ndc_xy = texCoord * 2.0 - 1.0;                                            // Convert texture coordinates to NDC space [-1, 1]
    
    // Calculate the width and height of the frustum at z_eye
    float aspect = viewport_size.x / viewport_size.y;
    float fov_rad = radians(fov);
    float half_height = tan(fov_rad * 0.5) * abs(z_eye);
    float half_width = half_height * aspect;
    
    // Calculate eye space position
    vec3 pos_eye;
    pos_eye.x = ndc_xy.x * half_width;
    pos_eye.y = ndc_xy.y * half_height;
    pos_eye.z = z_eye;
    
    return pos_eye;
}

// Simple noise function for fallback
float simpleNoise(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Get random rotation angle for this pixel
float getRandomRotation(vec2 texCoord) {
    if (use_randomization) {
        // Sample noise texture for rotation angle
        vec2 noiseCoord = texCoord * noise_scale;
        vec3 noise = texture(noise_texture, noiseCoord).xyz;
        // Use red channel for rotation angle
        return noise.r * 2.0 * PI;
    } else {
        return 0.0; // No randomization
    }
}

// SSAO calculation 
float calculateSSAO(vec2 texCoord, vec3 position, vec3 normal) {
    float occlusion = 0.0;
    int total_samples = 0;
    
    // Get random rotation for this pixel
    float randomRotation = getRandomRotation(texCoord);
    
    // Convert sample radius from world space to screen space
    vec4 offset = vec4(sample_radius, 0.0, position.z, 1.0);
    offset = projection * offset;
    float radius_screen = offset.x / offset.w;
    radius_screen = abs(radius_screen);
    
    // Circular sampling pattern with optional randomization
    for (int dir = 0; dir < num_directions; dir++) {
        // Base angle for this direction
        float base_angle = (float(dir) / float(num_directions)) * 2.0 * PI;
        
        // Add random rotation to reduce banding
        float angle = base_angle + randomRotation;
        
        vec2 direction = vec2(cos(angle), sin(angle));
        
        for (int sample_idx = 1; sample_idx <= samples_per_direction; sample_idx++) {
            // Calculate sample distance with optional jittering
            float base_distance = (float(sample_idx) / float(samples_per_direction)) * radius_screen;
            
            float distance = base_distance;
            if (use_randomization) {
                // Add slight jittering to sample distance to reduce regular patterns
                vec2 jitterCoord = texCoord + vec2(float(dir), float(sample_idx)) * 0.01;
                float jitter = simpleNoise(jitterCoord) * 0.2 - 0.1; // ±10% jitter
                distance = base_distance * (1.0 + jitter);
            }
            
            vec2 sample_coord = texCoord + direction * distance;
            
            // Check bounds - skip samples outside screen
            if (sample_coord.x < 0.0 || sample_coord.x > 1.0 || 
                sample_coord.y < 0.0 || sample_coord.y > 1.0) {
                continue;
            }
            
            // Sample depth and reconstruct position
            float sample_depth = texture(depth_texture, sample_coord).r;
            if (sample_depth >= 1.0) continue; // Skip background pixels
            
            vec3 sample_position = reconstructPosition(sample_coord, sample_depth);
            
            // Calculate vector from current position to sample
            vec3 sample_vector = sample_position - position;
            float sample_distance = length(sample_vector);
            vec3 sample_direction = normalize(sample_vector);
            
            // Check angle with normal (bias to reduce tangent surface artifacts)
            float dot_product = dot(normal, sample_direction);
            if (dot_product > bias_angle) {
                // Range check to avoid distant surfaces
                if (sample_distance < sample_radius) {
                    float attenuation = 1.0 - smoothstep(0.0, sample_radius, sample_distance);
                    occlusion += attenuation;
                }
            }
            
            total_samples++;
        }
    }
    
    // Normalize occlusion
    if (total_samples > 0) {
        occlusion = occlusion / float(total_samples);
    }
    
    // Clamp result
    occlusion = clamp(occlusion, 0.0, 1.0);
    
    // Return AO factor (0.0 = fully occluded, 1.0 = no occlusion)
    return 1.0 - occlusion;
}

void main()
{
    float depth = texture(depth_texture, v_uv).r;

    // Skip background pixels - output white (no occlusion)
    if (depth >= 1.0) {
        frag_color = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    vec3 normal = normalize(texture(normal_texture, v_uv).rgb);
    vec3 position = reconstructPosition(v_uv, depth);

    float ao = calculateSSAO(v_uv, position, normal);
    frag_color = vec4(ao, ao, ao, 1.0);
}