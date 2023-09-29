#version 330

uniform vec2 u_resolution;
#define u_color u_texture0
    uniform sampler2D u_texture0;
#define u_mesh_index u_texture2
    uniform isampler2D u_texture2;
uniform int u_outline_mesh_index;
uniform float u_outline_width;
uniform vec3 u_outline_color;

in vec2 v_texcoord;

layout(location = 0) out vec4 FragColor;

bool is_on_outline(vec2 center_pos, float width, int target_mesh_index)
{
    if (texture(u_mesh_index, center_pos).r == target_mesh_index) {
        return false;
    }
    const int SAMPLE_COUNT = 16;
    vec2 SAMPLES[SAMPLE_COUNT];     // Avoid array constructor for compatibility
    SAMPLES[0x0] = vec2(1.0, 0.0);
    SAMPLES[0x1] = vec2(-1.0, 0.0);
    SAMPLES[0x2] = vec2(0.0, 1.0);
    SAMPLES[0x3] = vec2(0.0, -1.0);
    SAMPLES[0x4] = vec2(0.707107, 0.707107);
    SAMPLES[0x5] = vec2(-0.707107, -0.707107);
    SAMPLES[0x6] = vec2(-0.707107, 0.707107);
    SAMPLES[0x7] = vec2(0.707107, -0.707107);
    SAMPLES[0x8] = vec2(0.382683, 0.92388);
    SAMPLES[0x9] = vec2(-0.382683, -0.92388);
    SAMPLES[0xA] = vec2(-0.92388, 0.382683);
    SAMPLES[0xB] = vec2(0.92388, -0.382683);
    SAMPLES[0xC] = vec2(0.92388, 0.382683);
    SAMPLES[0xD] = vec2(-0.92388, -0.382683);
    SAMPLES[0xE] = vec2(-0.382683, 0.92388);
    SAMPLES[0xF] = vec2(0.382683, -0.92388);
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        vec2 delta = width * SAMPLES[i] / u_resolution;
        int mesh_index = texture(u_mesh_index, center_pos + delta).r;
        if (mesh_index == target_mesh_index) {
            return true;
        }
    }
    return false;
}

void main()
{
    if (u_outline_mesh_index >= 0
        && is_on_outline(v_texcoord, u_outline_width, u_outline_mesh_index)) {
        FragColor = vec4(u_outline_color, 1.0);
    } else {
        FragColor = texture(u_color, v_texcoord);
    }
}
