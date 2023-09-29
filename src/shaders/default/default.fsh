#version 330

#define u_color_tex u_texture0
    uniform sampler2D u_texture0;
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

layout(location = 0) out vec4 FragColor;

//#include INCLUDE_SECTION_IMPLEMENTATION
#define INCLUDE_SECTION_IMPLEMENTATION
vec3 phong(PhongLamp lamp, PhongMaterial mat)
{
    vec3 ambient, diffuse, specular;
    // Ambient lighting
    ambient = mat.ambient * lamp.ambient;
    // Diffuse lighting
    vec3 normal = normalize(v_local_normal) * (gl_FrontFacing ? 1.0 : -1.0);
    vec3 incident = (lamp.mode == LAMP_MODE_SUN) ?
                    normalize(-lamp.pos) :
                    normalize(lamp.pos - v_local_pos);
    float fac_diffuse = max(dot(normal, incident), 0.0);
    diffuse = fac_diffuse * mat.diffuse * lamp.diffuse;
    // Specular lighting
    vec3 view_dir = normalize(u_camera_pos - v_local_pos);
    vec3 halfway = normalize(view_dir + incident);
    float fac_specular = pow(max(dot(normal, halfway), 0.0), mat.shininess);
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
    if (v_color.a * u_phong_material.alpha == 0.0 ||
        u_mesh_texcount > 0 && texture(u_color_tex, v_texcoord).a == 0.0) {
        discard;
    }
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0); // Keep alpha at 1.0
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
    FragColor = color;
}
