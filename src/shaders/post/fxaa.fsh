#version 330

uniform vec2 u_resolution;
uniform float u_gamma;
#define u_color u_texture0
    uniform sampler2D u_texture0;
#define u_depth u_texture1
    uniform sampler2D u_texture1;

in vec2 v_texcoord;

layout(location = 0) out vec4 FragColor;

float luma(vec3 rgba)
{
    return dot(rgba, vec3(0.299, 0.587, 0.114));
}

vec4 fxaa(vec2 pos, vec4 corners, sampler2D tex,
          vec4 inv_frame, vec4 inv_frame_2,
          float edge_sharpness, float edge_threshold, float edge_threshold_min)
{
    //--------------------------------------------------------------------------
    float luma_nw = luma(texture(tex, corners.xy).rgb);
    float luma_sw = luma(texture(tex, corners.xw).rgb);
    float luma_ne = luma(texture(tex, corners.zy).rgb);
    float luma_se = luma(texture(tex, corners.zw).rgb);
    //--------------------------------------------------------------------------
    vec4 rgby_m = texture(tex, pos.xy);
    float luma_m = luma(rgby_m.rgb);
    //--------------------------------------------------------------------------
    float luma_max_nw_sw = max(luma_nw, luma_sw);
    luma_ne += 1.0 / 384.0;
    float luma_min_nw_sw = min(luma_nw, luma_sw);
    //--------------------------------------------------------------------------
    float luma_max_ne_se = max(luma_ne, luma_se);
    float luma_min_ne_se = min(luma_ne, luma_se);
    //--------------------------------------------------------------------------
    float luma_max = max(luma_max_ne_se, luma_max_nw_sw);
    float luma_min = min(luma_min_ne_se, luma_min_nw_sw);
    //--------------------------------------------------------------------------
    float luma_max_scaled = luma_max * edge_threshold;
    //--------------------------------------------------------------------------
    float luma_min_m = min(luma_min, luma_m);
    float luma_max_scaled_clamped = max(edge_threshold_min, luma_max_scaled);
    float luma_max_m = max(luma_max, luma_m);
    float dir_sw_minus_ne = luma_sw - luma_ne;
    float luma_max_sub_min_m = luma_max_m - luma_min_m;
    float dir_se_minus_nw = luma_se - luma_nw;
    if(luma_max_sub_min_m < luma_max_scaled_clamped) {
        return rgby_m;
    }
    //--------------------------------------------------------------------------
    vec2 dir;
    dir.x = dir_sw_minus_ne + dir_se_minus_nw;
    dir.y = dir_sw_minus_ne - dir_se_minus_nw;
    //--------------------------------------------------------------------------
    vec2 dir_1 = normalize(dir.xy);
    vec4 rgby_n1 = texture(tex, pos.xy - dir_1 * inv_frame.zw);
    vec4 rgby_p1 = texture(tex, pos.xy + dir_1 * inv_frame.zw);
    //--------------------------------------------------------------------------
    float dir_abs_min_times_c = min(abs(dir_1.x), abs(dir_1.y)) *
                                edge_sharpness;
    vec2 dir_2 = clamp(dir_1.xy / dir_abs_min_times_c, -2.0, 2.0);
    //--------------------------------------------------------------------------
    vec2 dir_2_x = dir_2 * inv_frame_2.zw;
    vec4 rgby_n2 = texture(tex, pos.xy - dir_2_x);
    vec4 rgby_p2 = texture(tex, pos.xy + dir_2_x);
    //--------------------------------------------------------------------------
    vec4 rgby_a = rgby_n1 + rgby_p1;
    vec4 rgby_b = ((rgby_n2 + rgby_p2) * 0.25) + (rgby_a * 0.25);
    //--------------------------------------------------------------------------
    float luma_b = luma(rgby_b.rgb);
    if((luma_b < luma_min) || (luma_b > luma_max)) {
        rgby_b.xyz = rgby_a.xyz * 0.5;
    }
    return rgby_b;
}

void main()
{
    // corners {xy__} = upper left of pixel, {__zw} = lower right of pixel
    vec4 corners = vec4(
        v_texcoord - (0.6 / u_resolution),
        v_texcoord + (0.6 / u_resolution)
    );
    // inv_frame: N = 0.50 (default), N = 0.33 (sharper)
    vec4 inv_frame = vec4(
        vec2(-0.50, -0.50) / u_resolution,
        vec2(0.50, 0.50) / u_resolution
    );
    // inv_frame_2: N = 2;
    vec4 inv_frame_2 = vec4(
        vec2(-2.0, -2.0) / u_resolution,
        vec2(2.0, 2.0) / u_resolution
    );
    // Edge sharpness: 8.0 (sharp, default) - 2.0 (soft)
    float edge_sharpness = 8.0;
    // Edge threshold: 0.125 (softer, def) - 0.25 (sharper)
    float edge_threshold = 0.125;
    // 0.06 (faster, dark alias), 0.05 (def), 0.04 (slower, less dark alias)
    float edge_threshold_min = 0.05;

    FragColor = fxaa(v_texcoord, corners, u_color, inv_frame, inv_frame_2,
                     edge_sharpness, edge_threshold, edge_threshold_min);
}
