#version 330

#define u_color_tex u_texture0
    uniform sampler2D u_texture0;
uniform int u_mesh_texcount;

struct PhongMaterial {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float alpha;
};
uniform PhongMaterial u_phong_material;

in vec2 v_texcoord;
in vec4 v_color;

void main()
{
    if (v_color.a * u_phong_material.alpha == 0.0 ||
        u_mesh_texcount > 0 && texture(u_color_tex, v_texcoord).a == 0.0) {
        discard;
    }
}
