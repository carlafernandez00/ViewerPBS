#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normal_matrix;

out vec3 v_normal;
out vec2 v_uv;
out vec3 v_world_position;

void main(void)  {
    v_normal = mat3(transpose(inverse(model))) * normal; // normal in world space
    v_uv = texCoord;
     
    v_world_position = vec3(model * vec4( vert, 1.0f ));               // position of the vertex in world space
    gl_Position = projection * view * vec4(v_world_position, 1.0f);    // position of the vertex in clip space
}
