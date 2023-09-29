#version 330

uniform mat4 u_mat_ray_cast;
uniform vec2 u_resolution;
uniform vec3 u_camera_pos;
uniform vec3 u_clear_color;
uniform float u_grid_r = 100.0;
uniform float u_grid_f = 500.0;

in vec3 v_local_normal;
in vec3 v_local_pos;
in vec4 v_color;

layout(location = 0) out vec4 FragColor;

float fresnel_fac(float cos_t)
{
    const float n1 = 1.0, n2 = 1.3;
    const float f0 = pow((n1 - n2) / (n1 + n2), 2.0);
    return f0 + (1.0 - f0) * pow(1 - cos_t, 5.0);
}

vec3 ray_dir()
{
    vec2 ndc = gl_FragCoord.xy / u_resolution * vec2(2.0) - vec2(1.0);
    vec4 src = u_mat_ray_cast * vec4(ndc, -1.0, 1.0);
    vec4 dst = u_mat_ray_cast * vec4(ndc, 1.0, 1.0);
    src /= vec4(src.w);
    dst /= vec4(dst.w);
    return normalize(dst.xyz - src.xyz);
}

void main()
{
    float radius = length(v_local_pos.xz);
    float dist = clamp((radius - u_grid_r) / u_grid_f, 0.0, 1.0);
    float reflect = fresnel_fac(abs(dot(ray_dir(), v_local_normal)));
    float fade = clamp(dist + reflect, 0.0, 1.0);
    if (fade > 0.95) {
        discard;
    }
    float eff_alpha = (1.0 - fade) * v_color.a;
    FragColor = vec4(mix(u_clear_color, v_color.rgb, eff_alpha), 1.0);
}
