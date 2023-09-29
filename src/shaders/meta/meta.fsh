#version 330

uniform int u_mesh_index;
#define u_color_tex u_texture0
    uniform sampler2D u_texture0;
uniform int u_mesh_texcount;
uniform int u_colormap_count;
uniform vec4 u_colormap[16];

struct PhongMaterial {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float alpha;
};
uniform PhongMaterial u_phong_material;

in vec3 v_local_pos;
in vec2 v_texcoord;
in vec4 v_color;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec3 Position;
layout(location = 2) out int Index;

void main()
{
    if (v_color.a * u_phong_material.alpha == 0.0 ||
        u_mesh_texcount > 0 && texture(u_color_tex, v_texcoord).a == 0.0) {
        discard;
    }
    FragColor = u_colormap[u_mesh_index % u_colormap_count];
    Position = v_local_pos;
    Index = u_mesh_index;
}
