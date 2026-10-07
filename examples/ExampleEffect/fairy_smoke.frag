#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
    float iTime;
    vec2 iResolution;
};

void mainImage(out vec4 O, in vec2 I)
{
    float i = 0.0;
    float z = 0.0;
    float d = 0.0;
    float t = iTime;

    O = vec4(0.0);

    for (i = 0.0; i++ < 80.0; O += (cos(z + t + vec4(6.0, 1.0, 2.0, 0.0)) + 1.0) / d)
    {
        vec3 p = z * normalize(vec3(I + I, 0.0) - iResolution.xyy);
        p.z += 5.0;

        for (d = 1.0; d < 9.0; d /= 0.7)
            p += cos(p.yzx * d + t) / d;

        z += d = 0.01 + abs(length(p) - 2.0) / 7.0;
    }

    O = tanh(O / 3e3);
}

void main()
{
    // Convert Qt's normalized texcoords to Shadertoy-style pixel coords.
    // Flip Y because Shadertoy-style shaders often expect bottom-left origin.
    vec2 I = vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * iResolution;

    vec4 O;
    mainImage(O, I);

    fragColor = vec4(O.rgb * qt_Opacity, qt_Opacity);
}