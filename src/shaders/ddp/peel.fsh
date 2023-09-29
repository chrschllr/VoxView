#version 330

#define u_last_depth u_texture0
    uniform sampler2D u_texture0;
#define u_last_front_color u_texture1
    uniform sampler2D u_texture1;
#define u_color_tex u_texture2
    uniform sampler2D u_texture2;
uniform int u_mesh_texcount;

#define INCLUDE_SECTION_HEADER
#define LAMP_MODE_SUN 0
#define LAMP_MODE_POINT 1
struct PhongLamp {
    int mode; // 0 - Sun, 1 - Point
    vec3 pos;
    // Component colors/intensities
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    // Attenuation parameters
    float constant;
    float linear;
    float quadratic;
};
uniform PhongLamp u_lamps[8];
uniform int u_lamp_count;

struct PhongMaterial {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float alpha;
};
uniform PhongMaterial u_phong_material;

uniform vec3 u_camera_pos;
#undef INCLUDE_SECTION_HEADER

in vec3 v_normal;
in vec3 v_local_normal;
in vec3 v_local_pos;
in vec2 v_texcoord;
in vec4 v_color;

layout(location = 0) out vec2 Depth;
layout(location = 1) out vec4 FrontColor;
layout(location = 2) out vec4 BackColor;

const float INFINITY = 3.4028e38;

//#include INCLUDE_SECTION_IMPLEMENTATION
#define INCLUDE_SECTION_IMPLEMENTATION
vec3 phong(PhongLamp lamp, PhongMaterial mat)
{
    vec3 ambient, diffuse, specular;
    // Ambient lighting
    ambient = mat.ambient * lamp.ambient;
    // Diffuse lighting
    vec3 normal = normalize(v_local_normal);
    if (!gl_FrontFacing) normal *= -1.0;
    vec3 incident = (lamp.mode == LAMP_MODE_SUN) ?
                    normalize(lamp.pos) :
                    normalize(v_local_pos - lamp.pos);
    float fac_diffuse = max(dot(normal, -incident), 0.0);
    diffuse = fac_diffuse * mat.diffuse * lamp.diffuse;
    // Specular lighting
    vec3 view_dir = normalize(v_local_pos - u_camera_pos);
    vec3 reflected = reflect(incident, normal);
    float fac_specular = pow(max(dot(-view_dir, reflected), 0.0),
                             mat.shininess);
    specular = fac_specular * mat.specular * lamp.specular;
    // Combination
    vec3 combined = ambient + diffuse + specular;
    if (lamp.mode == LAMP_MODE_POINT) {
        float d = length(v_local_pos - lamp.pos);
        float attenuation = 1.0 / (lamp.constant + d *
                                  (lamp.linear + d * lamp.quadratic));
        combined *= attenuation;
    }
    return combined;
}
#undef INCLUDE_SECTION_IMPLEMENTATION

void main()
{
    float current_depth = gl_FragCoord.z;
    ivec2 frag_coord = ivec2(gl_FragCoord.xy);
    vec2 last_depth = texelFetch(u_last_depth, frag_coord, 0).rg;
    vec4 last_front_color = texelFetch(u_last_front_color, frag_coord, 0);

    Depth.rg = vec2(-INFINITY);
    FrontColor = last_front_color;
    BackColor = vec4(0.0);

    float nearest_depth = -last_depth.r;
    float farthest_depth = last_depth.g;
    if (current_depth < nearest_depth || current_depth > farthest_depth) return;
    if (current_depth > nearest_depth && current_depth < farthest_depth) {
        Depth.rg = vec2(-current_depth, current_depth);
        return;
    }

    vec4 color = vec4(0.0);
    // #include INCLUDE_SECTION_MAIN
    #define INCLUDE_SECTION_MAIN
    vec4 albedo = v_color;
    if (u_mesh_texcount > 0) {
        albedo *= texture(u_color_tex, v_texcoord);
    }
    // Phong lighting
    for (int i = 0; i < u_lamp_count; i++) {
        color.rgb += albedo.rgb * phong(u_lamps[i], u_phong_material);
    }
    #undef INCLUDE_SECTION_MAIN
    color.a = albedo.a * u_phong_material.alpha;

    if (current_depth == nearest_depth) {
        float alpha_multiplier = 1.0 - last_front_color.a;
        FrontColor.rgb += color.rgb * color.a * alpha_multiplier;
        FrontColor.a = 1.0 - alpha_multiplier * (1.0 - color.a);
    } else {
        BackColor += color;
    }
}
