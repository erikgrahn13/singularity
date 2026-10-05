#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 texelStep;
};

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec3 color = texture(source, qt_TexCoord0).rgb * 0.227027;
    color += texture(source, qt_TexCoord0 + texelStep * 1.384615).rgb * 0.316216;
    color += texture(source, qt_TexCoord0 - texelStep * 1.384615).rgb * 0.316216;
    color += texture(source, qt_TexCoord0 + texelStep * 3.230769).rgb * 0.070270;
    color += texture(source, qt_TexCoord0 - texelStep * 3.230769).rgb * 0.070270;

    fragColor = vec4(color * qt_Opacity, qt_Opacity);
}
