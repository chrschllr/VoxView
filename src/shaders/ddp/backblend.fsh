#version 330

#define u_back_color u_texture0
    uniform sampler2D u_texture0;

in vec2 v_texcoord;

layout(location = 0) out vec4 FragColor;

void main()
{
    ivec2 frag_coord = ivec2(gl_FragCoord.xy);
    FragColor = texelFetch(u_back_color, frag_coord, 0);
    if (FragColor.a == 0.0) discard;
}
