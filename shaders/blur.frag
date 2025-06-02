#version 330
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

// reconstruct position from depth and texture coordinates in eye space
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

// Noise function 
float simpleNoise(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Get random rotation vector
vec3 getRandomVector(vec2 texCoord) {
    if (use_randomization) {
        // Use noise texture for better randomization
        vec2 noiseCoord = texCoord * noise_scale;
        vec3 noise = texture(noise_texture, noiseCoord).xyz * 2.0 - 1.0;
        return normalize(noise);
    } else {
        // Fallback to simple noise
        float angle = simpleNoise(texCoord * 100.0) * 2.0 * PI;
        return vec3(cos(angle), sin(angle), 0.0);
    }
}

// SSAO calculation using circular kernel
float calculateSSAO(vec2 texCoord, vec3 position, vec3 normal) {
    float occlusion = 0.0;
    int total_samples = 0;
    
    // Get random rotation vector
    vec3 randomVec = getRandomVector(texCoord);
    
    // Create TBN matrix for oriented sampling
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Convert sample radius from world space to screen space
    vec4 offset = vec4(sample_radius, 0.0, position.z, 1.0);
    offset = projection * offset;
    float radius_screen = offset.x / offset.w;
    radius_screen = abs(radius_screen);
    
    // Sample in hemisphere oriented by normal
    for (int i = 0; i < num_directions * samples_per_direction; i++) {
        // Generate sample in hemisphere
        float angle = (float(i) / float(num_directions * samples_per_direction)) * 2.0 * PI;
        float radius = sqrt(float(i + 1) / float(num_directions * samples_per_direction));
        
        // Add some jittering
        vec2 jitter = vec2(simpleNoise(texCoord + float(i)), simpleNoise(texCoord + float(i) + 1000.0));
        angle += (jitter.x - 0.5) * 0.5;
        radius *= (0.8 + jitter.y * 0.4);
        
        vec3 sample_dir = vec3(cos(angle) * radius, sin(angle) * radius, sqrt(1.0 - radius * radius));
        sample_dir = TBN * sample_dir;
        
        // Project to screen space
        vec4 sample_pos_4d = vec4(position + sample_dir * sample_radius, 1.0);
        sample_pos_4d = projection * sample_pos_4d;
        vec2 sample_coord = (sample_pos_4d.xy / sample_pos_4d.w) * 0.5 + 0.5;
        
        // Check bounds
        if (sample_coord.x < 0.0 || sample_coord.x > 1.0 || 
            sample_coord.y < 0.0 || sample_coord.y > 1.0) {
            continue;
        }
        
        // Sample depth and reconstruct position
        float sample_depth = texture(depth_texture, sample_coord).r;
        if (sample_depth >= 1.0) continue;
        
        vec3 sample_position = reconstructPosition(sample_coord, sample_depth);
        
        // Calculate occlusion
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
        frag_color = vec4(1.0);
        return;
    }

    vec3 normal = normalize(texture(normal_texture, v_uv).rgb);
    vec3 position = reconstructPosition(v_uv, depth);

    float ao = calculateSSAO(v_uv, position, normal);
    frag_color = vec4(vec3(ao), 1.0);
    
}