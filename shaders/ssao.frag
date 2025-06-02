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
uniform int ao_algorithm;
uniform bool use_randomization;
uniform float bias_angle;

out vec4 frag_color;

const float PI = 3.14159265359;


vec3 reconstructPosition(vec2 texCoord, float depth) {
    // transform depth from [0, 1] to [-1, 1] NDC space and then to eye space
    float z_ndc = depth * 2.0 - 1.0;
    float z_eye = -(2.0 * zNear * zFar) / (zFar + zNear - z_ndc * (zFar - zNear));
    vec2 ndc_xy = texCoord * 2.0 - 1.0;
    
    float aspect = viewport_size.x / viewport_size.y;
    float fov_rad = radians(fov);
    float half_height = tan(fov_rad * 0.5) * zNear;  // At near plane
    float half_width = half_height * aspect; // At near plane

    float BC_x = ndc_xy.x * half_width;
    float x_eye = BC_x * abs(z_eye)/ zNear; // EF = BC * DE / AB 
    
    float BC_y = ndc_xy.y * half_height;
    float y_eye = BC_y * abs(z_eye)/ zNear; // EF = BC * DE / AB
  
    vec3 pos_eye;
    pos_eye.x = x_eye;
    pos_eye.y = y_eye;
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
        vec2 noiseCoord = texCoord * noise_scale;
        vec3 noise = texture(noise_texture, noiseCoord).xyz;
        return noise.r * 2.0 * PI;
    } else {
        return 0.0;
    }
}

// Compute tangent angle for HBAO 
float computeTangentAngle(vec3 normal, vec2 screen_direction, vec3 position) {
    // The tangent angle t(θ) is the signed elevation angle of the surface tangent vector
    
    // Project the normal onto the plane perpendicular to view direction
    vec3 view_dir = vec3(0.0, 0.0, 1.0); // View direction in eye space (+Z)
    vec3 projected_normal = normal - view_dir * dot(normal, view_dir);
    projected_normal = normalize(projected_normal);

    // Convert screen direction to eye space direction
    vec3 eye_direction = normalize(vec3(screen_direction, 0.0));
    
    // Calculate tangent angle: signed elevation angle of surface tangent
    float dot_product = clamp(dot(projected_normal, eye_direction), -1.0, 1.0);
    return asin(dot_product);
}

// HBAO horizon angle computation
float computeHorizonAngle(vec2 texCoord, vec3 position, vec3 normal, vec2 direction, float tangent_angle, out float attenuation) {
    float max_horizon_angle = tangent_angle; // Start with tangent angle
    attenuation = 0.0;
    
    // Convert sample radius from world space to screen space
    float radius_screen = sample_radius / abs(position.z) * projection[0][0] * 0.5;
    radius_screen = clamp(radius_screen, 0.001, 0.1);

    // Step along the direction
    for (int step = 1; step <= samples_per_direction; step++) {
        float step_size = (float(step) / float(samples_per_direction)) * radius_screen;
        
        // Apply jitter if randomization is enabled (as mentioned in paper)
        if (use_randomization) {
            vec2 jitterCoord = texCoord + direction * float(step) * 0.01;
            float jitter = simpleNoise(jitterCoord) * 0.2 - 0.1;
            step_size *= (1.0 + jitter);
        }

        vec2 sample_coord = texCoord + direction * step_size;

        // Check bounds
        if (sample_coord.x < 0.0 || sample_coord.x > 1.0 || 
            sample_coord.y < 0.0 || sample_coord.y > 1.0) {
            continue;
        }

        float sample_depth = texture(depth_texture, sample_coord).r;  
        if (sample_depth >= 1.0) continue;

        vec3 sample_position = reconstructPosition(sample_coord, sample_depth);
        vec3 D = sample_position - position; // D = Si - P 

        // Skip samples outside influence radius
        float sample_distance = length(D);
        if (sample_distance > sample_radius) continue;

        // elevation angle formula: α(Si) = atan(-D.z/||D.xy||)
        float horizontal_distance = length(D.xy);
        if (horizontal_distance > 0.001) {
            float elevation_angle = atan(-D.z / horizontal_distance);

            // h(θ) = max(t(θ), α(Si), i = 1..Ns)
            if (elevation_angle > max_horizon_angle) {
                max_horizon_angle = elevation_angle;
                // W(θ) = max(0, 1 - r(θ)/R)
                attenuation = max(0.0, 1.0 - sample_distance / sample_radius);
            }
        }
    }
    return max_horizon_angle;
}

// HBAO calculation -> paper's equation (3)
float calculateHBAO(vec2 texCoord, vec3 position, vec3 normal) {
    float totalAO = 0.0;
    float random_rotation = getRandomRotation(texCoord);
    
    // Paper uses Monte Carlo integration over Nd directions
    for (int dir = 0; dir < num_directions; dir++) {
        // Calculate direction angle
        float base_angle = (float(dir) / float(num_directions)) * 2.0 * PI;
        float angle = base_angle + random_rotation;
        vec2 direction = vec2(cos(angle), sin(angle));
        
        // Compute tangent angle for this direction
        float tangent_angle = computeTangentAngle(normal, direction, position);

        // Compute horizon angle
        float attenuation;
        float horizon_angle = computeHorizonAngle(texCoord, position, normal, direction, tangent_angle, attenuation);

        // A = 1 - (1/2π) ∫ (sin(h(θ)) - sin(t(θ)))W(θ)dθ -> Monte Carlo
        float sin_horizon = sin(clamp(horizon_angle, -PI/2.0, PI/2.0));
        float sin_tangent = sin(clamp(tangent_angle, -PI/2.0, PI/2.0));

        float ao_contribution = (sin_horizon - sin_tangent) * attenuation;
        totalAO += max(0.0, ao_contribution);
    }

    // Monte Carlo integration: (2π/Nd) * (1/2π) * sum = sum/Nd
    float ao = 1.0 - (totalAO / float(num_directions));
    return clamp(ao, 0.0, 1.0);
}


// SSAO calculation 
float calculateSSAO(vec2 texCoord, vec3 position, vec3 normal) {
    float occlusion = 0.0;
    int total_samples = 0;
    
    float randomRotation = getRandomRotation(texCoord);

    // convert sample radius from world space to screen space
    float radius_screen = sample_radius / abs(position.z) * projection[0][0] * 0.5;
    radius_screen = clamp(radius_screen, 0.001, 0.1);
    
    // circular kernel sampling 
    for (int dir = 0; dir < num_directions; dir++) {
        // Calculate equidistant direction angles
        float base_angle = (float(dir) / float(num_directions)) * 2.0 * PI;
        float angle = base_angle + randomRotation; // Add random rotation to the angle if enabled
        vec2 direction = vec2(cos(angle), sin(angle));
        
        // Sample along this direction at equidistant positions
        for (int sample_idx = 1; sample_idx <= samples_per_direction; sample_idx++) {
            float base_distance = (float(sample_idx) / float(samples_per_direction)) * radius_screen;
            float distance = base_distance;
            
            // Apply randomization to the distance if enabled with jitter
            if (use_randomization) {
                vec2 jitterCoord = texCoord + vec2(float(dir), float(sample_idx)) * 0.01;
                float jitter = simpleNoise(jitterCoord) * 0.2 - 0.1;
                distance = base_distance * (1.0 + jitter);
            }
            
            vec2 sample_coord = texCoord + direction * distance;
            
            // Handle fragments near border - skip samples outside the texture bounds
            if (sample_coord.x < 0.0 || sample_coord.x > 1.0 || 
                sample_coord.y < 0.0 || sample_coord.y > 1.0) {
                continue;
            }
            
            // Sample depth and reconstruct position
            float sample_depth = texture(depth_texture, sample_coord).r;
            if (sample_depth >= 1.0) continue;
            
            vec3 sample_position = reconstructPosition(sample_coord, sample_depth);
            
            vec3 sample_vector = sample_position - position;
            vec3 sample_direction = normalize(sample_vector);
            float sample_distance = length(sample_vector);

            // RANGE CHECK: Avoid that distant samples contribute
            if (sample_distance > sample_radius) continue;
            
            // Check if the sample is above the surface (> bias_angle)
            // Removing occlusion of nearly tangent surfaces
            float dot_product = dot(normal, sample_direction);
            if (dot_product > bias_angle) {
                // Attenuation to handle the discontinuities between samples
                float attenuation = 1.0 - smoothstep(0.0, sample_radius, sample_distance);
                
                occlusion += attenuation;
            }
            
            total_samples++;
        }
    }
    
    // Normalize and return AO factor (monte carlo average)
    if (total_samples > 0) {
        occlusion = occlusion / float(total_samples);
    }
    
    occlusion = clamp(occlusion, 0.0, 1.0);

    // Return inverted AO factor (1.0 = no occlusion, lower values = more occlusion)
    return 1.0 - occlusion;
}

void main()
{
    float depth = texture(depth_texture, v_uv).r;

    // Skip background pixels
    if (depth >= 1.0) {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0); 
        return;
    }

    // Sample G-buffer data - reconstruct position to eye space
    vec3 normal = normalize(texture(normal_texture, v_uv).rgb * 2.0 - 1.0);
    vec3 position = reconstructPosition(v_uv, depth);

    float ao;

    // Spherical Sampling
    if (ao_algorithm == 0) {
        ao = calculateSSAO(v_uv, position, normal);
    }
    // Horizon-Based Sampling
    else if (ao_algorithm == 1) {
        ao = calculateHBAO(v_uv, position, normal);
    }

    // Output AO value (white = no occlusion, black = full occlusion)
    frag_color = vec4(ao, ao, ao, 1.0);
}