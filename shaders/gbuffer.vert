#version 330

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normal_matrix;

smooth out vec3 v_normal;
out vec2 v_uv;

void main(void) {
    vec4 view_vertex = view * model * vec4(vertex, 1);
    v_normal = normalize(normal_matrix * normal);
    v_uv = texCoord;

    gl_Position = projection * view_vertex;
}
