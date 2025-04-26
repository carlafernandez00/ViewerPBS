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

    // Remove translation from view matrix -> so that the skybox is not moved when the 
    // camera moves (illusion of being far away)
    mat4 sky_view = view;
    sky_view[3][0] = 0.0f;
    sky_view[3][1] = 0.0f;
    sky_view[3][2] = 0.0f;

    gl_Position = projection * sky_view * vec4(vert, 1.0f);  // position of the vertex in clip space
}
