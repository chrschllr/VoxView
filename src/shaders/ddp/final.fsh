#version 330

#define u_front_color u_texture0
    uniform sampler2D u_texture0;
#define u_back_color u_texture1
    uniform sampler2D u_texture1;

in vec2 v_texcoord;

layout(location = 0) out vec4 FragColor;

void main()
{
    ivec2 frag_coord = ivec2(gl_FragCoord.xy);
    vec4 back_color = texelFetch(u_back_color, frag_coord, 0);
    vec4 front_color = texelFetch(u_front_color, frag_coord, 0);
    float alpha_multiplier = 1.0 - front_color.a;

    FragColor = vec4(
        front_color.rgb + alpha_multiplier * back_color.rgb,
        front_color.a + back_color.a
    );
}
