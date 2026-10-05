#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
    float bloomIntensity;
};

layout(binding = 1) uniform sampler2D source;
layout(binding = 2) uniform sampler2D halfBloomSource;
layout(binding = 3) uniform sampler2D quarterBloomSource;
layout(binding = 4) uniform sampler2D eighthBloomSource;

vec3 toneMap(vec3 color)
{
    vec3 numerator = color * (2.51 * color + 0.03);
    vec3 denominator = color * (2.43 * color + 0.59) + 0.14;
    return clamp(numerator / denominator, 0.0, 1.0);
}

void main()
{
    vec3 original = texture(source, qt_TexCoord0).rgb;
    vec3 bloom = texture(halfBloomSource, qt_TexCoord0).rgb * 0.5;
    bloom += texture(quarterBloomSource, qt_TexCoord0).rgb * 0.3;
    bloom += texture(eighthBloomSource, qt_TexCoord0).rgb * 0.2;
    vec3 color = toneMap(original + bloom * bloomIntensity);

    fragColor = vec4(color * qt_Opacity, qt_Opacity);
}
