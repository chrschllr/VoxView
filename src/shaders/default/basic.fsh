#version 330

in vec3 v_normal;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec3 color = normalize(v_normal) * vec3(0.5, 0.5, 0.5) + vec3(0.5);
    FragColor = vec4(color, 1.0);
}
