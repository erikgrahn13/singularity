#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
    float threshold;
};

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec3 color = texture(source, qt_TexCoord0).rgb;
    float brightness = max(max(color.r, color.g), color.b);
    float contribution = max(brightness - threshold, 0.0) / max(brightness, 0.0001);
    vec3 brightColor = color * contribution;

    fragColor = vec4(brightColor * qt_Opacity, qt_Opacity);
}
