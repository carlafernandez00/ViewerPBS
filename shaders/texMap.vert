#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 v_uv;

void main(void)  {
    v_uv = texCoord;
    
    vec3 v_world_position = vec3(model * vec4( vert, 1.0f ));               // position of the vertex in world space
    gl_Position = projection * view * vec4(v_world_position, 1.0f);  
}
