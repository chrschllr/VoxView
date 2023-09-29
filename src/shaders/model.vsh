#version 330

uniform mat4 u_mat_model;
uniform mat4 u_mat_view;
uniform mat4 u_mat_projection;
uniform mat4 u_mat_local_normal;
uniform mat4 u_mat_normal;
uniform float u_gamma;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;
layout(location = 3) in vec4 a_color;

out vec3 v_normal;
out vec3 v_local_normal;
out vec3 v_local_pos;
out vec2 v_texcoord;
out vec4 v_color;

// https://learnopengl.com/img/getting-started/coordinate_systems.png

void main()
{
    // Normal & position
    v_normal = mat3(u_mat_normal) * a_normal;
    v_local_normal = mat3(u_mat_local_normal) * a_normal;
    v_local_pos = vec3(u_mat_model * vec4(a_position, 1.0));
    // Texture and color
    v_texcoord = a_texcoord;
    v_color = vec4(pow(a_color.rgb, vec3(u_gamma)), a_color.a);
    // Projection
    gl_Position = u_mat_projection * u_mat_view * vec4(v_local_pos, 1.0);
}
