#version 330

uniform vec2 u_resolution;
uniform float u_gamma;
#define u_color u_texture0
    uniform sampler2D u_texture0;
#define u_depth u_texture1
    uniform sampler2D u_texture1;

in vec2 v_texcoord;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 color = texture(u_color, v_texcoord);
    FragColor = vec4(pow(color.rgb, vec3(1.0 / u_gamma)), 1.0);
    // Uncomment the following lines to visualize the depth buffer
    // float z_arg = 0.02 / ((texture(u_depth, v_texcoord).r) - 1.0);
    // vec3 z_col = vec3(cos(z_arg), cos(z_arg + 2.094), cos(z_arg - 2.094));
    // FragColor.rgb = vec3(0.5) + vec3(0.5) * z_col;
}
