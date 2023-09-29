#version 330

layout(location = 0) in vec3 a_position;

out vec2 v_texcoord;

void main()
{
    v_texcoord = vec2(0.5) * a_position.xy + vec2(0.5);
    gl_Position = vec4(a_position, 1.0);
}
