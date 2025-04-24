#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 v_position;

void main(void)  {
    v_position = vert; 
    gl_Position = projection * view * vec4(vert, 1.0f);  // position of the vertex in view space
}
