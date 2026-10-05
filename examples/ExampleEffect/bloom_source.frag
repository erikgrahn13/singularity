#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
    float iTime;
    vec2 iResolution;
    float hdrBoost;
    float pulseWidth;
};

vec3 rainbow(float position)
{
    const float tau = 6.28318530718;
    return 0.55 + 0.45 * cos(tau * (position + vec3(0.0, 0.67, 0.33)));
}

void main()
{
    vec2 uv = qt_TexCoord0;
    vec2 point = uv * 2.0 - 1.0;
    point.x *= iResolution.x / max(iResolution.y, 1.0);

    vec3 color = vec3(0.004, 0.002, 0.008);
    float waveCenter = sin(point.x * 2.8 + iTime * 1.15) * 0.3;
    waveCenter += sin(point.x * 5.1 - iTime * 0.55) * 0.055;
    float line = 1.0 - smoothstep(0.0, 0.022, abs(point.y - waveCenter));

    float pulsePosition = fract(iTime * 0.18);
    float pulseDistance = abs(uv.x - pulsePosition);
    pulseDistance = min(pulseDistance, 1.0 - pulseDistance);
    float normalizedDistance = pulseDistance / max(pulseWidth, 0.001);
    float superbrightPulse = exp(-normalizedDistance * normalizedDistance * 2.0);
    float brightness = 1.0 + superbrightPulse * hdrBoost;

    color += rainbow(uv.x + iTime * 0.035) * line * 2.4 * brightness;

    for (int index = 0; index < 9; ++index)
    {
        float dotPosition = (float(index) + 0.5) / 9.0;
        float alternatingRow = mod(float(index), 2.0);
        float dotY = 0.18 + 0.64 * alternatingRow;
        dotY += sin(iTime * 1.2 + float(index)) * 0.025;
        vec2 dotDelta = (uv - vec2(dotPosition, dotY))
                        * vec2(iResolution.x / max(iResolution.y, 1.0), 1.0);
        float dotShape = 1.0 - smoothstep(0.012, 0.025, length(dotDelta));
        color += rainbow(dotPosition + iTime * 0.035) * dotShape * 2.0;
    }

    fragColor = vec4(color * qt_Opacity, qt_Opacity);
}
