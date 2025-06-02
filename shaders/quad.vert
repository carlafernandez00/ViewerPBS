#version 330
layout (location = 0) in vec2 vertex;
layout (location = 2) in vec2 texture_coords;

out vec2 v_uv;

void main()
{
    v_uv = texture_coords;
    gl_Position = vec4(vertex.x, vertex.y, 0.0, 1.0);
}